#include <gtest/gtest.h>
#include <cucumber-cpp/defs.hpp>

#include <string>
#include <map>

class ActiveActors {
public:
    typedef std::string actor_name_type;
    typedef unsigned short actor_year_type;
private:
    typedef std::map<actor_name_type, actor_year_type> actors_type;

    actors_type actors;
public:
    void addActor(const actor_name_type name, const actor_year_type year) {
        actors[name] = year;
    }

    void retireActor(const actor_name_type name) {
        actors.erase(actors.find(name));
    }

    actor_name_type getOldestActor() {
        actors_type::iterator it = actors.begin();
        actor_name_type name = it->first;
        actor_year_type year = it->second;
        while(++it != actors.end()) {
            actor_year_type currentYear = it->second;
            if (year > currentYear) {
                name = it->first;
            }
        }
        return name;
    }
};

GIVEN("^the following actors are still active") {
    TABLE_PARAM(actorsParam);
    USING_CONTEXT(ActiveActors, context);

    const table_hashes_type & actors = actorsParam.hashes();
    for (table_hashes_type::const_iterator ait = actors.begin(); ait != actors.end(); ++ait) {
        std::string name(ait->at("name"));
        std::string yearString(ait->at("born"));
        const ActiveActors::actor_year_type year = ::cuke::internal::fromString<ActiveActors::actor_year_type>(yearString);
        context->addActor(name, year);
    }
}

WHEN("^(.+) retires") {
    REGEX_PARAM(std::string, retiringActor);
    USING_CONTEXT(ActiveActors, context);

    context->retireActor(retiringActor);
}

THEN("^the oldest actor should be (.+)$") {
    REGEX_PARAM(std::string, oldestActor);
    USING_CONTEXT(ActiveActors, context);

    ASSERT_EQ(oldestActor, context->getOldestActor());
}
