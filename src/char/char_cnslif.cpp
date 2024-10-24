// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "char_cnslif.hpp"

#include <cstdlib>
#include <cstring>

#include <common/cli.hpp>
#include <common/ers.hpp>
#include <common/showmsg.hpp>
#include <common/socket.hpp>
#include <common/timer.hpp>

#include "char.hpp"

/*======================================================
 * Login-Server help option info
 *------------------------------------------------------*/
void display_helpscreen(bool do_exit)
{
	ShowInfo("Uso: %s [options]\n", SERVER_NAME);
	ShowInfo("\n");
	ShowInfo("Opcoes:\n");
	ShowInfo(" -?, -h [--help]\t\tExibe esta tela de ajuda.\n");
	ShowInfo(" -v [--version]\t\tExibe a versao do servidor.\n");
	ShowInfo(" --run-once\t\t\tFecha o servidor apos o carregamento (teste).\n");
	ShowInfo(" --char-config <file>\t\tConfiguracao alternativa do char-server.\n");
	ShowInfo(" --lan-config <file>\t\tConfiguracao alternativa do lag.\n");
	ShowInfo(" --inter-config <file>\t\tConfiguracao alternativa entre servidores.\n");
	ShowInfo(" --msg-config <file>\t\tConfiguracao de mensagem alternativa.\n");
	if( do_exit )
		exit(EXIT_SUCCESS);
}

/**
 * Timered function to check if the console has a new event to be read.
 * @param tid: timer id
 * @param tick: tick of execution
 * @param id: user account id
 * @param data: unused
 * @return 0
 */
TIMER_FUNC(cnslif_console_timer){
	char buf[MAX_CONSOLE_IN]; //max cmd atm is 63+63+63+3+3

	memset(buf,0,MAX_CONSOLE_IN); //clear out buf

	if(cli_hasevent()){
		if(fgets(buf, MAX_CONSOLE_IN, stdin)==nullptr)
			return -1;
		else if(strlen(buf)>MIN_CONSOLE_IN)
			cnslif_parse(buf);
	}
	return 0;
}

// Console Command Parser [Wizputer]
int cnslif_parse(const char* buf)
{
	char type[64];
	char command[64];
	int n=0;

	if( ( n = sscanf(buf, "%63[^:]:%63[^\n]", type, command) ) < 2 ){
		if((n = sscanf(buf, "%63[^\n]", type))<1) return -1; //nothing to do no arg
	}
	if( n != 2 ){ //end string
		ShowNotice("Tipo: '%s'\n",type);
		command[0] = '\0';
	}
	else
		ShowNotice("Tipo de comando: '%s' || Comando: '%s'\n",type,command);

	if( n == 2 && strcmpi("server", type) == 0 ){
		if( strcmpi("shutdown", command) == 0 || strcmpi("exit", command) == 0 || strcmpi("quit", command) == 0 ){
			global_core->signal_shutdown();
		}
		else if( strcmpi("alive", command) == 0 || strcmpi("status", command) == 0 )
			ShowInfo(CL_CYAN "Console: " CL_BOLD "Estou Online." CL_RESET "\n");
		else if( strcmpi("reloadconf", command) == 0 ) {
			ShowInfo("Re-carregando config\"%s\"\n", CHAR_CONF_NAME);
			char_config_read(CHAR_CONF_NAME, false);
		}
	}
	else if( strcmpi("ers_report", type) == 0 ){
		ers_report();
	}
	else if( strcmpi("help", type) == 0 ){
		ShowInfo("Comandos disponiveis:\n");
		ShowInfo("\t server:shutdown => Para o servidor.\n");
		ShowInfo("\t server:alive => Verifica se o servidor esta em execucao.\n");
		ShowInfo("\t server:reloadconf => Recarrega o arquivo de configuracao: \"%s\"\n", CHAR_CONF_NAME);
		ShowInfo("\t ers_report => Exibe o uso do banco de dados.\n");
	}

	return 0;
}

void do_init_chcnslif(void){
	if( charserv_config.console ){ //start listening
		add_timer_func_list(cnslif_console_timer, "cnslif_console_timer");
		add_timer_interval(gettick()+1000, cnslif_console_timer, 0, 0, 1000); //start in 1s each 1sec
	}
}
