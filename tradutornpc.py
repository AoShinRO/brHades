import os
import re
import time
import logging
import codecs
import sys
from tqdm import tqdm
from googletrans import Translator
from loguru import logger

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Caminho para a pasta de traduções
pasta_traducao = "db/translated"

SERVICE_URLS = ['translate.google.com']

# Atualizando a expressão regular para ignorar qualquer string entre colchetes
ignore_npcname_pattern = r'\[.*?\]'  # Captura qualquer string entre colchetes, incluindo números e símbolos

# Regex para extrair os comandos de mes, select etc...
mes_pattern = re.compile(r'mes\s+"([^"]+)"')
npctalk_pattern = re.compile(r'npctalk\s+"([^"]+)"(?:\s*,\s*"([^"]*)")?(?:\s*,\s*(\d+))?(?:\s*,\s*\^?([A-Fa-f0-9]{6}))?;')
announce_pattern = re.compile(r'announce\s+"([^"]+)",\s*\d+(?:,\s*(#[A-Fa-f0-9]{6})(?:,\s*(\w+)(?:,\s*(\d+)(?:,\s*(\w+)(?:,\s*(\d+)(?:,\s*(\d+))?)?)?)?)?)?;')
mapannounce_pattern = re.compile(r'mapannounce\s+"([^"]+)",\s*"([^"]+)",\s*\d+(?:,\s*(#[A-Fa-f0-9]{6})(?:,\s*(\w+)(?:,\s*(\d+)(?:,\s*(\w+)(?:,\s*(\d+))?)?)?)?)?;')
areaannounce_pattern = re.compile(r'areaannounce\s+"([^"]+)",\s*\d+,\s*\d+,\s*\d+,\s*\d+,\s*"([^"]+)",\s*\d+(?:,\s*(#[A-Fa-f0-9]{6})(?:,\s*(\w+)(?:,\s*(\d+)(?:,\s*(\w+)(?:,\s*(\d+))?)?)?)?)?;')
instance_announce_pattern = re.compile(r'instance_announce\s+\d+,\s*"([^"]+)",\s*\d+(?:,\s*(#[A-Fa-f0-9]{6})(?:,\s*(\w+)(?:,\s*(\d+)(?:,\s*(\w+)(?:,\s*(\d+))?)?)?)?)?;')
select_pattern = re.compile(r'select\s*\(\s*"([^"]+)"\s*\)')

# Regex para extrair os comandos de color, `
pattern_tags = r"<[^>]+>[^<]*<\/[^>]+>"
pattern_color = r"\^([A-Fa-f0-9]{6})[^\^]*"

# Configurações do logger e da API de tradução
TRANSLATORS = {
    "GoogleTranslator": Translator(service_urls=SERVICE_URLS)
}

CODEPAGE_BY_LANGUAGE = {
    "en": "utf-8",  # Inglês - Universalmente UTF-8
    "ru": "cp1251",  # Russo - Codepage Windows para russo
    "es": "cp1252",  # Espanhol - Codepage Windows para espanhol
    "de": "cp1252",  # Alemão - Codepage Windows para alemão
    "zh-CN": "gbk",  # Chinês Simplificado
    "mg": "utf-8",  # Malaio - UTF-8 é amplamente suportado
    "id": "utf-8",  # Indonésio
    "fr": "cp1252",  # Francês
    "pt": "cp1252",  # Português Brasileiro
    "th": "tis-620",  # Tailandês
}

traducoes_existentes = {}

def configure_logger(level="INFO"):
    """
    Configura o logger Loguru para exibir logs no console com codificação UTF-8.
    """
    utf8_stdout = codecs.getwriter("utf-8")(sys.stdout.buffer)
    logger.remove()  # remove o logger padrão
    logger.add(utf8_stdout, format="'\n'{time: YYYY-MM-DD - HH:mm:ss} - {message}'\n'", level=level)

def traduzir_texto_com_padroes(texto, origem='en', destino='pt'):
    """
    Traduz o texto enquanto preserva padrões específicos, como tags HTML e códigos de cores.
    """
    ignored_parts = []

    placeholder_text = texto
    placeholder_offset = 0

    if texto == " ":
        return texto

    if texto == "Sure!":
        return "Certo!"

    if texto == "Sure":
        return "Certo"

    if texto in traducoes_existentes:
        return traducoes_existentes[texto]

    # Encontrando e ignorando partes que precisam ser preservadas
    for match in re.finditer(f"{pattern_tags}|{pattern_color}|{ignore_npcname_pattern}", texto):
        start, end = match.span()
        ignored_text = match.group()
        ignored_parts.append(ignored_text)

        # Substituindo o texto encontrado por um marcador único
        marker = f"_{len(ignored_parts) - 1}"
        placeholder_text = (
                placeholder_text[:start + placeholder_offset] +
                marker +
                placeholder_text[end + placeholder_offset:]
        )
        placeholder_offset += len(marker) - len(ignored_text)

    # Traduzindo o texto (sem os padrões)
    translated_text = traduzir_texto(placeholder_text, origem=origem, destino=destino)

    # Recolocando os padrões de volta no texto traduzido
    for i, original_text in enumerate(ignored_parts):
        translated_text = translated_text.replace(f"_{i}", original_text)

    return translated_text

def dividir_texto(texto, max_len=3000):
    """
    Divide o texto em pedaços menores para garantir que cada parte não ultrapasse o limite de caracteres.
    """
    return [texto[i:i + max_len] for i in range(0, len(texto), max_len)]

def traduzir_texto(texto, origem='en', destino='pt'):
    """
    Traduz o texto utilizando o tradutor configurado, dividindo o texto se necessário.
    """
    tentativas = 3  # Exemplo: número de tentativas
    espera_retentiva = 1
    pausa_long_delay = 3

    # Dividir o texto se ele for maior que 3000 caracteres
    partes_texto = dividir_texto(texto)

    texto_traduzido = ""
    if texto == " ":
        return texto

    if texto in traducoes_existentes:
        return traducoes_existentes[texto]

    for parte in partes_texto:
        parte_traduzida = ""  # Armazena a tradução da parte atual
        for tentativa in range(1, tentativas + 1):
            try:
                time.sleep(espera_retentiva)
                # Tentando a tradução
                for translator in TRANSLATORS.keys():
                    translated_text = TRANSLATORS[translator].translate(parte, dest=destino).text
                    if translated_text:  # Verifique se a tradução é válida
                        parte_traduzida = translated_text
                        break  # Sai do loop de tradutores se uma tradução for bem-sucedida
                
                if parte_traduzida:  # Se a tradução foi bem-sucedida, sai do loop de tentativas
                    break

            except Exception as e:
                logger.error(f"Tentativa {tentativa} de tradução falhou: {e}")
                if tentativa < tentativas:
                    logger.info(f"Aguardando {pausa_long_delay} segundos para tentar novamente.")
                    time.sleep(pausa_long_delay)

        if parte_traduzida:  # Adiciona apenas se a tradução foi bem-sucedida
            texto_traduzido += parte_traduzida
        else:
            logger.error(f"Falha ao traduzir a parte: '{parte}' após várias tentativas.")
            texto_traduzido += parte  # Mantém a parte original se a tradução falhar

    if not texto_traduzido:
        logger.error("Tradução não foi concluída após várias tentativas.")
        return texto
    return texto_traduzido

def processar_arquivo(arquivo, destino_dir, origem='en', destino='pt'):
    """
    Processa o arquivo, traduzindo seu conteúdo e preservando a estrutura.
    """
    logger.debug(f"Processando arquivo: {arquivo}")
    try:
        with open(arquivo, 'r', encoding='utf-8') as f:
            conteudo = f.read()
    except UnicodeDecodeError as e:
        try:
            with open(arquivo, 'r', encoding='cp1252') as f:
                conteudo = f.read()
        except UnicodeDecodeError as e:
            logger.error(f"Erro ao ler o arquivo '{arquivo}': {e}")
            return

    # Contar linhas para a barra de progresso
    linhas = conteudo.splitlines()
    total_linhas = len(linhas)

    conteudo_processado = ""
    # Barra de progresso para a tradução de linhas
    with tqdm(total=total_linhas, desc=f"Traduzindo {arquivo}", unit="linha") as pbar:
        for linha in linhas:
            if mes_pattern.search(linha) or select_pattern.search(linha) or npctalk_pattern.search(linha) or announce_pattern.search(linha) or mapannounce_pattern.search(linha) or areaannounce_pattern.search(linha) or instance_announce_pattern.search(linha):
                conteudo_processado = traduzir_patterns(linha, origem, destino)
                salvar_traducao(linha, conteudo_processado, destino)
            pbar.update(1)  # Atualiza a barra de progresso após cada linha processada

    registrar_arquivo_processado(os.path.relpath(arquivo, start=destino_dir))

def traduzir_patterns(linha, origem, destino):
    """
    Substitui as mensagens e opções de mes, select e similares por suas versões traduzidas.
    """
    nova_linha = ""

    # Tradução do comando mes
    for match in mes_pattern.findall(linha):
        texto_traduzido = traduzir_texto_com_padroes(match, origem, destino)
        nova_linha += f"{match}|{destino}|{texto_traduzido}\n"

    # Tradução do comando select
    for match in select_pattern.findall(linha):
        opcoes = match.split(":")    
        opcoes_traduzidas = [traduzir_texto_com_padroes(opcao, origem, destino) for opcao in opcoes]
        nova_linha += f"{match}|{destino}|{':'.join(opcoes_traduzidas)}\n"

    # Tradução do comando npctalk
    for match in npctalk_pattern.findall(linha):
        texto_a_traduzir = match[0]
        texto_traduzido = traduzir_texto(texto_a_traduzir, origem, destino)
        nova_linha += f"{texto_a_traduzir}|{destino}|{texto_traduzido}\n"

    # Tradução do comando announce
    for match in announce_pattern.findall(linha):
        texto_a_traduzir = match[0]
        texto_traduzido = traduzir_texto(texto_a_traduzir, origem, destino)
        nova_linha += f"{texto_a_traduzir}|{destino}|{texto_traduzido}\n"

    # Tradução do comando mapannounce
    for match in mapannounce_pattern.findall(linha):
        texto_a_traduzir = match[1]
        texto_traduzido = traduzir_texto(texto_a_traduzir, origem, destino)
        nova_linha += f"{texto_a_traduzir}|{destino}|{texto_traduzido}\n"

    # Tradução do comando areaannounce
    for match in areaannounce_pattern.findall(linha):
        texto_a_traduzir = match[1]
        texto_traduzido = traduzir_texto(texto_a_traduzir, origem, destino)
        nova_linha += f"{texto_a_traduzir}|{destino}|{texto_traduzido}\n"

    # Tradução do comando instance_announce
    for match in instance_announce_pattern.findall(linha):
        texto_a_traduzir = match[0]
        texto_traduzido = traduzir_texto(texto_a_traduzir, origem, destino)
        nova_linha += f"{texto_a_traduzir}|{destino}|{texto_traduzido}\n"

    return nova_linha

def salvar_traducao(mensagem_original, traducao, idioma):
    """
    Salva a tradução no arquivo correspondente ao idioma no formato desejado.
    """

    if mensagem_original in traducoes_existentes:
        return  # Já traduzido

    nome_arquivo = f"database_{idioma}.lua"
    caminho_arquivo = os.path.join(pasta_traducao, nome_arquivo)

    # Garante que a pasta exista
    os.makedirs(pasta_traducao, exist_ok=True)

    # Escreve no arquivo no formato correto
    with open(caminho_arquivo, 'a', encoding=CODEPAGE_BY_LANGUAGE.get(idioma, "utf-8")) as f:
        f.write(traducao)

    traducaosplit = traducao.strip().split('|')
    mensagem_original, idioma, traducao = traducaosplit
    traducoes_existentes[mensagem_original] = traducao

def carregar_traducoes_existentes(destino):
    """
    Carrega mensagens já traduzidas de um arquivo existente para evitar duplicatas.
    """
    nome_arquivo = f"database_{destino}.lua"
    arquivo_destino = os.path.join(pasta_traducao, nome_arquivo)
    if os.path.exists(arquivo_destino):
        with open(arquivo_destino, 'r', encoding=CODEPAGE_BY_LANGUAGE.get(destino, "utf-8")) as f:
            for linha in f:
                partes = linha.strip().split('|')
                if len(partes) == 3:  # Verifique o formato esperado
                    mensagem_original, idioma, traducao = partes
                    traducoes_existentes[mensagem_original] = traducao

def registrar_arquivo_processado(nome_arquivo, destino='pt'):
    """
    Registra o nome do arquivo processado no arquivo externo.
    """
    arquivo_processados = os.path.join(pasta_traducao, f"arquivos_processados_{destino}.txt")
    with open(arquivo_processados, "a", encoding="utf-8") as f:
        f.write(nome_arquivo + "\n")

def verificar_arquivos_faltantes(pasta_origem, pasta_destino, destino='pt'):
    """
    Verifica quais arquivos ainda precisam ser processados.
    """
    arquivo_processados = os.path.join(pasta_traducao, f"arquivos_processados_{destino}.txt")
    arquivos_origem = {
        os.path.relpath(os.path.join(dirpath, f), start=pasta_origem)
        for dirpath, _, files in os.walk(pasta_origem) for f in files if f.endswith(".txt")
    }
    if os.path.exists(arquivo_processados):
        with open(arquivo_processados, "r", encoding="utf-8") as f:
            arquivos_processados = set(f.read().splitlines())
    else:
        arquivos_processados = set()

    return list(arquivos_origem - arquivos_processados)

def main(destino='pt'):
    pasta_npc = 'npc'
    
    # Verifica se a pasta de origem existe
    if not os.path.exists(pasta_npc):
        logger.error(f"A pasta de origem '{pasta_npc}' não foi encontrada.")
        return

    # Verifica quais arquivos precisam ser processados
    arquivos_faltantes = verificar_arquivos_faltantes(pasta_npc, pasta_traducao, destino)
    carregar_traducoes_existentes(destino)

    # Se não houver arquivos faltantes, encerra o processo
    if not arquivos_faltantes:
        logger.info("Todos os arquivos já foram processados.")
        return

    logger.info(f"Iniciando a tradução de {len(arquivos_faltantes)} arquivos da pasta '{pasta_npc}' para '{pasta_traducao}'.")

    # Barra de progresso com a lista de arquivos
    with tqdm(total=len(arquivos_faltantes), desc="Traduzindo NPCs", unit="arquivo") as pbar:
        for arquivo_relativo in arquivos_faltantes:
            arquivo = os.path.join(pasta_npc, arquivo_relativo)
            try:
                logger.debug(f"Iniciando o processamento do arquivo: {arquivo}")
                processar_arquivo(arquivo, pasta_traducao)
                pbar.update(1)  # Atualiza a barra de progresso após cada arquivo processado
                logger.debug(f"Arquivo processado com sucesso: {arquivo}")
            except Exception as e:
                logger.error(f"Erro ao processar o arquivo '{arquivo}': {e}")

    logger.info("Processo de tradução concluído.")

if __name__ == "__main__":
    main()