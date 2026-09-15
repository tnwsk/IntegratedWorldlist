#include "IntegratedDemonlist.hpp"
#include <jasmine/web.hpp>

using namespace geode::prelude;

std::vector<IDListDemon> IntegratedDemonlist::demonlist;
bool IntegratedDemonlist::demonlistLoaded = false;

void IntegratedDemonlist::loadDemonlist(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://list.smart-game.lat/api/v2/demons/listed/"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());

            demonlistLoaded = true;
            demonlist.clear();

            for (auto& level : jasmine::web::getArray(res)) {
                auto id = level.get<int>("level_id");
                if (!id.isOk()) continue;

                auto position = level.get<int>("position");
                if (!position.isOk()) continue;

                auto name = level.get<std::string>("name");
                if (!name.isOk()) continue;

                IDListDemon demon(id.unwrap(), position.unwrap(), std::move(name).unwrap());

                demonlist.insert(std::ranges::upper_bound(demonlist, demon, [](const IDListDemon& a, const IDListDemon& b) {
                    return a.position < b.position;
                }), std::move(demon));
            }

            success();
        }
    );
}
