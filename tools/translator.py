import codecs
import re
import sys

from deep_translator import GoogleTranslator   
from loguru import logger

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

def configure_logger(level="INFO"):
    """
    Configures the Loguru logger to output UTF-8 encoded logs to console.
    """
    utf8_stdout = codecs.getwriter("utf-8")(sys.stdout.buffer)

    logger.remove()  # remove the default logger

    logger.add(utf8_stdout, format="{time: YYYY-MM-DD - HH:mm:ss} {level} - <level>{message}</level>", level=level)


def translate_text(Translator, text, source='en', target='pt'):
    """
        Attempts to translate text from source to target language using available translators.

        :param text: The text to translate
        :param source: Code for the source language (default: 'en')
        :param target: Code for the target language (default: 'pt')
        :return: Translated text or original text if translation fails
        """
    try:
        return Translator.translate(text)
    except Exception:
        logger.info(f"Erro ao tentar traduzir de {text} para {target}")

    return text  # Retorna o texto original se todas as tentativas falharem


def translate_with_patterns(Translator, text, source='en', target='pt'):
    """
    Translates text while preserving specific patterns like HTML tags and color codes.

    :param text: The text to translate
    :param source: Code for the source language
    :param target: Code for the target language
    :return: Translated text with patterns preserved
    """
    pattern_tags = r"<[^>]+>[^<]*<\/[^>]+>"
    pattern_color = r"\^([A-Fa-f0-9]{6})[^\^]*\^000000"

    ignored_parts = []

    placeholder_text = text
    placeholder_offset = 0

    for match in re.finditer(f"{pattern_tags}|{pattern_color}", text):
        start, end = match.span()
        ignored_text = match.group()
        ignored_parts.append(ignored_text)

        # Replace the matched text with a unique placeholder
        marker = f"_{len(ignored_parts) - 1}"
        placeholder_text = (
                placeholder_text[:start + placeholder_offset] +
                marker +
                placeholder_text[end + placeholder_offset:]
        )
        placeholder_offset += len(marker) - len(ignored_text)

    translated_text = translate_text(Translator, placeholder_text, source=source, target=target)

    for i, original_text in enumerate(ignored_parts):
        translated_text = translated_text.replace(f"_{i}", original_text)

    return translated_text


if __name__ == "__main__":
    if len(sys.argv) < 4:
        sys.exit(1)

    configure_logger("INFO")

    text_to_translate = sys.argv[1]
    source_lang = sys.argv[2]
    target_lang = sys.argv[3]

    Translator = GoogleTranslator(source=source_lang, target=target_lang) 

    translation = translate_with_patterns(Translator, text_to_translate, source=source_lang, target=target_lang)

    # Output the translation with the specified encoding
    with codecs.getwriter(CODEPAGE_BY_LANGUAGE.get(target_lang))(sys.stdout.buffer) as encoded_stdout:
        encoded_stdout.write(translation)
