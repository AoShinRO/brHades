import sys
import re
from googletrans import Translator
from deep_translator import GoogleTranslator, MyMemoryTranslator

codepage_por_idioma = {
    "en": "utf-8",         # Inglês - Universalmente UTF-8
    "ru": "cp1251",        # Russo - Codepage Windows para russo
    "es": "cp1252",        # Espanhol - Codepage Windows para espanhol
    "de": "cp1252",        # Alemão - Codepage Windows para alemão
    "zh-CN": "gbk",           # Chinês Simplificado
    "mg": "utf-8",         # Malaio - UTF-8 é amplamente suportado
    "id": "utf-8",         # Indonésio
    "fr": "cp1252",        # Francês
    "pt": "cp1252",        # Português Brasileiro
    "th": "tis-620",       # Tailandês
}

def definir_codepage(idioma):
    return sys.stdout.reconfigure(encoding=codepage_por_idioma.get(idioma))

def traduzir(texto, origem='en', destino='pt'):
    tradutores = [
        ('Google'),
        ('GoogleTranslator'),
        ('MyMemory'),
    ]
    for nome in tradutores:
        try:
            if nome == 'Google':
                translator = Translator(service_urls=['translate.google.com'])
                traducao = translator.translate(texto, src=origem, dest=destino)
                return traducao.text
            elif nome == 'GoogleTranslator':
                translator = GoogleTranslator()
                traducao = translator.translate(texto, target=destino)
                return traducao
            elif nome == 'MyMemory':
                translator = MyMemoryTranslator()
                traducao = translator.translate(texto, target=destino)
                return traducao
        except Exception as e:
            continue
    else:
        return texto  # Retorna o texto original se todas as tentativas falharem

def traduzir_com_padroes(texto, origem='en', destino='pt'):
    pattern_tags = r"<[^>]+>[^<]*<\/[^>]+>"
    pattern_color = r"\^([A-Fa-f0-9]{6})[^\^]*\^000000"

    ignored_parts = []
    
    def replace_with_marker(match):
        ignored_parts.append(match.group())
        return f"_{len(ignored_parts)-1}"

    temp_text = re.sub(pattern_tags, replace_with_marker, texto)
    temp_text = re.sub(pattern_color, replace_with_marker, temp_text)
    
    translated_text = traduzir(temp_text, origem=origem, destino=destino)
    
    for i, original_text in enumerate(ignored_parts):
        translated_text = translated_text.replace(f"_{i}", original_text)
    
    return translated_text

if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(1)

    texto_para_traduzir = sys.argv[1]
    lingua_origem = sys.argv[2]
    lingua_destino = sys.argv[3]
    codepage = definir_codepage(lingua_destino)
    
    traducao = traduzir_com_padroes(texto_para_traduzir, origem=lingua_origem, destino=lingua_destino)

    print(traducao) 
