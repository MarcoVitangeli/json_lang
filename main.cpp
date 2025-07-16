#include <iostream>
#include <utility>
#include <vector>
#include "simdjson.h"

enum TokenType : char {
    Root,
    Property,
    BracketExpression,
};

class Token {
    const std::string content;
    TokenType type;
public:
    explicit Token(std::string& content, const TokenType type = Root) : content(std::move(content)), type(type) {}
    explicit Token(const char c, const TokenType type = Root) : content(std::to_string(c)), type(type) {}

    [[nodiscard]] TokenType get_type() const { return type; }
    [[nodiscard]] std::string_view get_content() const { return content; }
};

class Tokenizer {
    const std::string& source;
    size_t index;

    char consume() { return source[index++]; }

    [[nodiscard]] char peak() const { return source[index]; }

    [[nodiscard]] bool eof() const { return index >= source.size(); }

    void try_consume_root(std::vector<Token>& tokens) {
        // TODO: allow $ to be part of a property name
        if (index != 0) throw std::runtime_error("root code found");
        tokens.emplace_back(consume());
    }

    void try_consume_property(std::vector<Token>& tokens) {
        consume();
        std::string prop_name;
        while (!eof() && (std::isalpha(peak()) || peak() == '_')) {
            prop_name += consume();
        }

        if (prop_name.empty()) {
            throw std::runtime_error("empty property name after dot");
        }

        tokens.emplace_back(prop_name, Property);
    }

    void try_consume_bracket_expression(std::vector<Token>& tokens) {
        consume();

        std::string expr;

        while (!eof() && peak() != ']') {
            expr += consume();
        }

        std::cout << expr << std::endl;

        if (peak() != ']') {
            throw std::runtime_error("end of bracket expression not found");
        }

        if (expr.empty()) {
            throw std::runtime_error("empty expression after bracket");
        }
        consume();

        // TODO: compile this expression at lexing time
        tokens.emplace_back(expr, BracketExpression);
    }

public:
    explicit Tokenizer(const std::string& source) : source(source), index(0) {}

    [[nodiscard]] std::vector<Token> get_tokens() {
        std::vector<Token> tokens;

        while (!eof()) {
            switch (peak()) {
                case '$': {
                    try_consume_root(tokens);
                    break;
                }
                case '.': {
                    try_consume_property(tokens);
                    break;
                }
                case '[': {
                    try_consume_bracket_expression(tokens);
                    break;
                }
                default:
                    throw std::runtime_error("unexpected character found at index " + std::to_string(index) + " with value " + std::to_string(peak()));
            }
        }

        return tokens;
    }
};

class Parser {
    simdjson::ondemand::document& doc;
    const std::vector<Token>& tokens;
    size_t index;

    Token consume() { return tokens[index++]; }
    [[nodiscard]] Token peak() const { return tokens[index]; }
    [[nodiscard]] bool eof() const { return index >= tokens.size(); }

    void ensure_valid_root() {
        if (index != 0) {
            throw std::runtime_error("$ has to be the first token in the expression");
        }
        consume();
    }
public:
    explicit Parser(std::vector<Token>& tokens, simdjson::ondemand::document& doc) : doc(doc), tokens(tokens), index(0) {}

    std::string_view parse() {
        auto obj = doc.get_value();
        if (obj.error()) {
            throw std::runtime_error("error while parsing document to json object: " + obj.error());
        }

        auto curr_obj = obj.value();
        while (!eof()) {
            switch (peak().get_type()) {
                case Root: {
                    ensure_valid_root();
                    break;
                }
                case Property: {
                    switch (curr_obj.type()) {
                        case simdjson::ondemand::json_type::string:
                        case simdjson::ondemand::json_type::null:
                        case simdjson::ondemand::json_type::number:
                        case simdjson::ondemand::json_type::boolean: {
                            throw std::runtime_error("ERROR: cannot access propery methods of scalar value. Property: " + std::string(peak().get_content()));
                        }
                        case simdjson::ondemand::json_type::array: {
                            throw std::runtime_error("ERROR: cannot access property of array, use bracket expressions instead. Property: " + std::string(peak().get_content()));
                        }
                        default: break;
                    }
                    const auto prop = consume();
                    const auto prop_name = prop.get_content();
                    const auto res = curr_obj.find_field(prop_name).value();

                    curr_obj = res;
                    break;
                }
                case BracketExpression: {
                    std::cout << peak().get_content() << std::endl;
                    throw std::domain_error("feature non-implemented");
                }
                default: {
                    throw std::runtime_error("Unknown token type found while parsing");
                }
            }
        }
        return simdjson::to_json_string(curr_obj);
    }
};

int main() {
    simdjson::ondemand::parser json_parser;
    const auto json = simdjson::padded_string::load("input.json");

    if (json.error()) {
        std::cerr << json.error() << std::endl;
        return -1;
    }

    simdjson::ondemand::document doc = json_parser.iterate(json);
    const std::string source = "$.user.locations[.name == 'Buenos Aires']";

    auto tokenizer = Tokenizer(source);
    auto tokens = tokenizer.get_tokens();

    auto json_lang = Parser(tokens, doc);

    const auto res = json_lang.parse();

    std::cout << res << std::endl;

    return 0;
}