#include <cucumber-cpp/internal/hook/Tag.hpp>

namespace cuke {
namespace internal {

Regex & AndTagExpression::csvTagNotationRegex() {
    static Regex r("\\s*\"([^\"]+)\"\\s*(?:,|$)");
    return r;
}

AndTagExpression::AndTagExpression(const std::string &csvTagNotation) {
    const shared_ptr<RegexMatch> match(csvTagNotationRegex().findAll(csvTagNotation));
    const RegexMatch::submatches_type submatches = match->getSubmatches();
    for (RegexMatch::submatches_type::const_iterator i = submatches.begin(); i != submatches.end(); ++i) {
        const std::string orCsvTagNotation = i->value;
        shared_ptr<OrTagExpression> orExpression(new OrTagExpression(orCsvTagNotation));
        orExpressions.push_back(orExpression);
    }
}

bool AndTagExpression::matches(const tag_list &tags) {
    bool match = true;
    for (or_expressions_type::const_iterator i = orExpressions.begin(); i != orExpressions.end() && match; ++i) {
        match &= (*i)->matches(tags);
    }
    return match;
}


Regex & OrTagExpression::csvTagNotationRegex() {
    static Regex r("\\s*@(\\w+)\\s*(?:,|$)");
    return r;
}

OrTagExpression::OrTagExpression(const std::string &csvTagNotation) {
    const shared_ptr<RegexMatch> match(csvTagNotationRegex().findAll(csvTagNotation));
    const RegexMatch::submatches_type submatches = match->getSubmatches();
    for (RegexMatch::submatches_type::const_iterator i = submatches.begin(); i != submatches.end(); ++i) {
        orTags.push_back(i->value);
    }
}

bool OrTagExpression::matches(const tag_list &tags) {
    bool match = false;
    for (tag_list::const_iterator i = orTags.begin(); i != orTags.end() && !match; ++i) {
        match = orTagMatchesTagList(*i, tags);
    }
    return match;
}

bool OrTagExpression::orTagMatchesTagList(const std::string &currentOrTag, const tag_list &tags) {
    bool match = false;
    for (tag_list::const_iterator i = tags.begin(); i != tags.end() && !match; ++i) {
        match = ((*i) == currentOrTag);
    }
    return match;
}

}
}
