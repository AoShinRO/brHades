// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "database.hpp"

#include <iostream>
#include <sstream>

#include "malloc.hpp"
#include "showmsg.hpp"
#include "utilities.hpp"

using namespace brhades;

bool YamlDatabase::nodeExists( const ryml::NodeRef& node, std::string_view name ){
	return (node.num_children() > 0 && node.has_child(c4::to_csubstr(name.data())));
}

bool YamlDatabase::nodesExist( const ryml::NodeRef& node, std::initializer_list<const std::string> names ){
	bool missing = false;

	for( std::string_view name : names ){
		if( !this->nodeExists( node, name ) ){
			ShowError( "Missing mandatory node \"'%.*s'\".\n", (int)name.size(), name.data() );
			missing = true;
		}
	}

	if( missing ){
		this->invalidWarning( node, "At least one mandatory node did not exist.\n" );
		return false;
	}

	return true;
}

bool YamlDatabase::verifyCompatibility( const ryml::Tree& tree ){
	if( !this->nodeExists( tree.rootref(), "Header" ) ){
		ShowError( "No database \"Header\" was found.\n" );
		return false;
	}

	const ryml::NodeRef& headerNode = tree["Header"];

	std::string tmpType;

	if( !this->asString( headerNode, "Type", tmpType ) ){
		return false;
	}

	if( this->type != tmpType ){
		ShowError( "Database type mismatch: %s != %s.\n", this->type.c_str(), tmpType.c_str() );
		return false;
	}

	uint16 tmpVersion;

	if( !this->asUInt16( headerNode, "Version", tmpVersion ) ){
		return false;
	}

	if( tmpVersion != this->version ){
		if( tmpVersion > this->version ){
			ShowError( "Database version %hu is not supported. Maximum version is: %hu\n", tmpVersion, this->version );
			return false;
		}else if( tmpVersion >= this->minimumVersion ){
			ShowWarning( "Database version %hu is outdated and should be updated. Current version is: %hu\n", tmpVersion, this->version );
			ShowWarning( "Reduced compatibility with %s database file from '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), this->currentFile.c_str() );
		}else{
			ShowError( "Database version %hu is not supported anymore. Minimum version is: %hu\n", tmpVersion, this->minimumVersion );
			return false;
		}
	}

	return true;
}

bool YamlDatabase::load(){
	bool ret = this->load( this->getDefaultLocation() );

	this->loadingFinished();

	return ret;
}

bool YamlDatabase::reload(){
	this->clear();

	return this->load();
}

static std::pair<size_t, std::unique_ptr<char[]>> readDBFileAsync(std::string_view  filepath, const std::string& type) {

	std::FILE* f = std::fopen(filepath.data(), "rb");
	if (f == nullptr) 
		return { 0,nullptr };
	
	std::fseek(f, 0, SEEK_END);
	size_t len = std::ftell(f);
	std::unique_ptr<char[]> buf(new char[len + 1]);
	std::rewind(f);
	size_t real_size = std::fread(buf.get(), sizeof(char), len, f);
	// Zero terminate
	buf[real_size] = '\0';

	if (std::ferror(f)) 
		return { 0,nullptr };
	
	std::fclose(f);
	return std::make_pair(len, std::move(buf));
}

bool YamlDatabase::load(std::string_view  path) {
	ShowStatus("Carregando '" CL_WHITE "%.*s" CL_RESET "'..." CL_CLL "\r", (int)path.size(), path.data());

	auto futureResult = std::async(std::launch::async, readDBFileAsync, path, this->type);
	auto result = futureResult.get(); // Wait for file reading to finish
	size_t size = result.first;
	if(!size)
	{
		ShowError("Falha ao abrir o arquivo de banco de dados %s de '" CL_WHITE "'%.*s'" CL_RESET "'.\n", this->type.c_str(), (int)path.size(), path.data());
		return false;
	}
	std::unique_ptr<char[]> buf = std::move(result.second);

	parser = {};
	ryml::Tree tree;

	try {
		tree = parser.parse_in_arena(c4::to_csubstr(path.data()), c4::to_csubstr(buf.get()));
	}
	catch (const std::runtime_error& e) {
		ShowError( "Falha ao carregar o arquivo de banco de dados %s de '" CL_WHITE "'%.*s'" CL_RESET "'.\n", this->type.c_str(), (int)path.size(), path.data() );
		ShowError( "Provavelmente ha um erro de sintaxe no arquivo.\n" );
		ShowError( "Mensagem de erro: %s\n", e.what() );
		return false;
	}

	// Required here already for header error reporting
	this->currentFile = path;

	if (!this->verifyCompatibility(tree)) {
		ShowError("Falha ao verificar a compatibilidade com o arquivo de banco de dados %s de '" CL_WHITE "%s" CL_RESET "'.\n", this->type.c_str(), this->currentFile.c_str());
		return false;
	}

	const ryml::NodeRef& header = tree["Header"];

	if (this->nodeExists(header, "Clear")) {
		bool clear;

		if (this->asBool(header, "Clear", clear) && clear) {
			this->clear();
		}
	}

	this->parse(tree);

	this->parseImports(tree);

	return true;
}

void YamlDatabase::loadingFinished(){
	// Does nothing by default, just for hooking
}

void YamlDatabase::parse( const ryml::Tree& tree ){
	uint64 count = 0;

	if( this->nodeExists( tree.rootref(), "Body" ) ){
		const ryml::NodeRef& bodyNode = tree["Body"];
		const char* fileName = this->currentFile.c_str();

#ifdef DETAILED_LOADING_OUTPUT
		size_t childNodesCount = bodyNode.num_children();
		ShowStatus("Carregando entradas de '" CL_WHITE "%" PRIdPTR CL_RESET "' em '" CL_WHITE "%s" CL_RESET "'\n", childNodesCount, fileName);
#endif

		for( const ryml::NodeRef &node : bodyNode ){
			count += this->parseBodyNode( node );
#ifdef DETAILED_LOADING_OUTPUT
			ShowStatus( "Carregando [%" PRIu64 "/%" PRIdPTR "] entradas de '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\r", count, childNodesCount, fileName );
#endif
		}

		ShowStatus( "Concluida a leitura de '" CL_WHITE "%" PRIu64 CL_RESET "' entradas em '" CL_WHITE "%s" CL_RESET "'" CL_CLL "\n", count, fileName );
	}
}

void YamlDatabase::parseImports( const ryml::Tree& rootNode ){
	if( this->nodeExists( rootNode.rootref(), "Footer" ) ){
		const ryml::NodeRef& footerNode = rootNode["Footer"];

		if( this->nodeExists( footerNode, "Imports") ){
			const ryml::NodeRef& importsNode = footerNode["Imports"];

			for( const ryml::NodeRef &node : importsNode ){
				std::string importFile;

				if( !this->asString( node, "Path", importFile ) ){
					continue;
				}

				if( this->nodeExists( node, "Mode" ) ){
					std::string mode;

					if( !this->asString( node, "Mode", mode ) ){
						continue;
					}

#ifdef RENEWAL
					std::string compiledMode = "Renewal";

					// RENEWAL mode with RENEWAL_ASPD off, load pre-re ASPD
#ifndef RENEWAL_ASPD
					if (importFile.find("job_aspd.yml") != std::string::npos)
						compiledMode = "Prerenewal";
#endif
#else
					std::string compiledMode = "Prerenewal";
#endif

					if( compiledMode != mode ){
						// Skip this import
						continue;
					}
				}

				if (this->nodeExists(node, "Generator")) {
					bool isGenerator;
					if (!this->asBool(node, "Generator", isGenerator)) {
						continue;
					}
					if (!(shouldLoadGenerator && isGenerator))
						continue; // skip import
				}

				this->load( importFile );
			}
		}
	}
}

template <typename R> bool YamlDatabase::asType( const ryml::NodeRef& node, std::string_view  name, R& out ){
	if( this->nodeExists( node, name ) ){
		const ryml::NodeRef& dataNode = node[c4::to_csubstr(name.data())];

		if (dataNode.val_is_null()) {
			this->invalidWarning(node, "Node \"'%.*s'\" is missing a value.\n", (int)name.size(), name.data());
			return false;
		}

		try{
			dataNode >> out;
		}catch( std::runtime_error const& ){
			this->invalidWarning( node, "Node \"'%.*s'\" cannot be parsed as %s.\n", (int)name.size(), name.data(), typeid( R ).name() );
			return false;
		}

		return true;
	}else{
		this->invalidWarning( node, "Missing node \"'%.*s'\".\n", (int)name.size(), name.data() );
		return false;
	}
}

bool YamlDatabase::asBool(const ryml::NodeRef& node, std::string_view  name, bool &out) {
	const ryml::NodeRef& targetNode = node[c4::to_csubstr(name.data())];

	if (targetNode.val_is_null()) {
		this->invalidWarning(node, "Node \"'%.*s'\" is missing a value.\n", (int)name.size(), name.data());
		return false;
	}

	std::string str;
	
	targetNode >> str;

	util::tolower( str );

	if( str == "true" ){
		out = true;
		return true;
	}else if( str == "false" ){
		out = false;
		return true;
	}else{
		this->invalidWarning( targetNode, "Unknown boolean value: \"%s\".\n", str.c_str() );
		return false;
	}
}

bool YamlDatabase::asInt16( const ryml::NodeRef& node, std::string_view  name, int16& out ){
	return asType<int16>( node, name, out);
}

bool YamlDatabase::asUInt16(const ryml::NodeRef& node, std::string_view  name, uint16& out) {
	return asType<uint16>(node, name, out);
}

bool YamlDatabase::asInt32(const ryml::NodeRef& node, std::string_view  name, int32 &out) {
	return asType<int32>(node, name, out);
}

bool YamlDatabase::asUInt32(const ryml::NodeRef& node, std::string_view  name, uint32 &out) {
	return asType<uint32>(node, name, out);
}

bool YamlDatabase::asInt64(const ryml::NodeRef& node, std::string_view  name, int64 &out) {
	return asType<int64>(node, name, out);
}

bool YamlDatabase::asUInt64(const ryml::NodeRef& node, std::string_view  name, uint64 &out) {
	return asType<uint64>(node, name, out);
}

bool YamlDatabase::asFloat(const ryml::NodeRef& node, std::string_view  name, float &out) {
	return asType<float>(node, name, out);
}

bool YamlDatabase::asDouble(const ryml::NodeRef& node, std::string_view  name, double &out) {
	return asType<double>(node, name, out);
}

bool YamlDatabase::asString(const ryml::NodeRef& node, std::string_view  name, std::string &out) {
	return asType<std::string>(node, name, out);
}

bool YamlDatabase::asUInt16Rate( const ryml::NodeRef& node, std::string_view  name, uint16& out, uint16 maximum ){
	if( this->asUInt16( node, name, out ) ){
		if( out > maximum ){
			this->invalidWarning( node[c4::to_csubstr(name.data())], "Node \"'%.*s'\" with value %" PRIu16 " exceeds maximum of %" PRIu16 ".\n", (int)name.size(), name.data(), out, maximum );

			return false;
		}else if( out == 0 ){
			this->invalidWarning( node[c4::to_csubstr(name.data())], "Node \"'%.*s'\" needs to be at least 1.\n", (int)name.size(), name.data() );

			return false;
		}else{
			return true;
		}
	}else{
		return false;
	}
}

bool YamlDatabase::asUInt32Rate( const ryml::NodeRef& node, std::string_view  name, uint32& out, uint32 maximum ){
	if( this->asUInt32( node, name, out ) ){
		if( out > maximum ){
			this->invalidWarning( node[c4::to_csubstr(name.data())], "Node \"'%.*s'\" with value %" PRIu32 " exceeds maximum of %" PRIu32 ".\n", (int)name.size(), name.data(), out, maximum );

			return false;
		}else if( out == 0 ){
			this->invalidWarning( node[c4::to_csubstr(name.data())], "Node \"'%.*s'\" needs to be at least 1.\n", (int)name.size(), name.data() );

			return false;
		}else{
			return true;
		}
	}else{
		return false;
	}
}

int32 YamlDatabase::getLineNumber(const ryml::NodeRef& node) {
	return parser.source().has_str() ? (int32)parser.location(node).line : 0;
}

int32 YamlDatabase::getColumnNumber(const ryml::NodeRef& node) {
	return parser.source().has_str() ? (int32)parser.location(node).col : 0;
}

void YamlDatabase::invalidWarning( const ryml::NodeRef& node, const char* fmt, ... ){
	va_list ap;

	va_start(ap, fmt);

	// Remove any remaining garbage of a previous loading line
	ShowMessage( CL_CLL );
	// Print the actual error
	_vShowMessage( MSG_ERROR, fmt, ap );

	va_end(ap);

	ShowError( "Occurred in file '" CL_WHITE "%s" CL_RESET "' on line %d and column %d.\n", this->currentFile.c_str(), this->getLineNumber(node), this->getColumnNumber(node));

#ifdef DEBUG
	std::cout << node;
#endif
}

std::string YamlDatabase::getCurrentFile(){
	return this->currentFile;
}

void YamlDatabase::setGenerator(bool shouldLoad) {
	shouldLoadGenerator = shouldLoad;
}

void on_yaml_error( const char* msg, size_t len, ryml::Location loc, void *user_data ){
	throw std::runtime_error( msg );
}

void do_init_database(){
	ryml::set_callbacks( ryml::Callbacks( nullptr, nullptr, nullptr, on_yaml_error ) );
}
