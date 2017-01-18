#include <cassert>
#include <ctype.h>
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace std;

namespace {
    enum TokenCategory {
        NEWLINE,
        WHITESPACE,
        END,
        IDENTIFIER,
        OTHER,
    };

    struct Token {
        TokenCategory category;
        string text;
    };

    class Lexer {

    public:
        Lexer();
        ~Lexer();
        bool load(const char *path);
        void unload();
        Token next();

    private:
        FILE *m_file;

    };
}

Lexer::Lexer() : m_file(nullptr) {
}

Lexer::~Lexer() {
    if (m_file)
        unload();
}

bool Lexer::load(const char *path) {
    if (m_file)
        unload();
    m_file = fopen(path, "r");
    return m_file != nullptr;
}

void Lexer::unload() {
    if (m_file)
        fclose(m_file);
    m_file = nullptr;
}

static bool is_identifier_char(char c) {
    return isalnum(c) || c == '_' || c == '.' || c == '$' || c == '@';
}

static bool is_whitespace_char(char c) {
    return isspace(c) && c != '\n';
}

Token Lexer::next() {
    assert(m_file != NULL);

    Token token;

    int c = fgetc(m_file);

    if (c == '\n') {
        token.category = NEWLINE;
        token.text = "\n";
        return token;
    }

    else if (isspace(c)) {
        token.category = WHITESPACE;
        token.text += char(c);
        while (is_whitespace_char(c = fgetc(m_file)))
            token.text += char(c);
    }

    else if (c == EOF) {
        token.category = END;
        return token;
    }

    else if (isalpha(c) || c == '_' || c == '.') {
        token.category = IDENTIFIER;
        token.text += char(c);
        while (is_identifier_char(c = fgetc(m_file)))
            token.text += char(c);
    }

    else {
        token.category = OTHER;
        token.text = char(c);
        return token;
    }

    /* If we've read one character beyond the current token, skip back so we can
     * retrieve it next time.
     */
    if (c != EOF)
        (void)fseek(m_file, -1, SEEK_CUR);

    return token;
}
