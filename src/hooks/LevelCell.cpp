#include "../IntegratedDemonlist.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/StringBuffer.hpp>
#include <jasmine/hook.hpp>
#include <jasmine/setting.hpp>

using namespace geode::prelude;

std::set<int> loadedDemons;

class $modify(IDLevelCell, LevelCell) {
    struct Fields {
        TaskHolder<web::WebResponse> m_listener;
    };

    static void onModify(ModifyBase<ModifyDerive<IDLevelCell, LevelCell>>& self) {
        jasmine::hook::modify(self.m_hooks, "LevelCell::loadFromLevel", "enable-rank");
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);

        if (level->m_levelType == GJLevelType::Editor || level->m_demon.value() <= 0 ||
            level->isPlatformer() || level->m_demonDifficulty < 6) return;

        auto levelID = level->m_levelID.value();
        auto position = 0;
        for (auto& demon : IntegratedDemonlist::demonlist) {
            if (demon.id == levelID) {
                position = demon.position;
                break;
            }
        }
        if (position != 0) return addRank(position);

        if (loadedDemons.contains(levelID)) return;
        loadedDemons.insert(levelID);

        m_fields->m_listener.spawn(
            web::WebRequest().get(fmt::format("https://list.smart-game.lat/api/levels/{}", levelID)),
            [this, levelID, levelName = std::string(level->m_levelName)](web::WebResponse res) mutable {
                if (!res.ok()) return;

                auto jsonArray = res.json();
                if (!jsonArray.isOk()) return;

                auto json = std::move(jsonArray.unwrap().get(0));
                if (!json.isOk()) return;

                auto positionRes = json.unwrap().get<int>("position");
                if (!positionRes.isOk()) return;

                auto position = positionRes.unwrap();
                IDListDemon demon(levelID, position, std::move(levelName));
                if (!std::ranges::contains(IntegratedDemonlist::demonlist, demon)) {
                    IntegratedDemonlist::demonlist.push_back(std::move(demon));
                }

                addRank(position);
            }
        );
    }

    void addRank(int position) {
        if (m_mainLayer->getChildByID("level-rank-label"_spr)) return;

        auto dailyLevel = m_level->m_dailyID.value() > 0;
        auto isWhite = dailyLevel || jasmine::setting::getValue<bool>("white-rank");

        auto rankTextNode = CCLabelBMFont::create(fmt::format("#{} The World Demonlist", position).c_str(), "chatFont.fnt");
        rankTextNode->setPosition({ 346.0f, m_height - 1.0f });
        rankTextNode->setAnchorPoint({ 1.0f, 1.0f });
        rankTextNode->setScale(m_compactView ? 0.45f : 0.6f);
        auto rlc = Loader::get()->getLoadedMod("raydeeux.revisedlevelcells");
        if (rlc && rlc->getSettingValue<bool>("enabled") && rlc->getSettingValue<bool>("blendingText")) {
            rankTextNode->setBlendFunc({ GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA });
        }
        else if (isWhite) {
            rankTextNode->setOpacity(152);
        }
        else {
            rankTextNode->setColor({ 51, 51, 51 });
            rankTextNode->setOpacity(200);
        }
        rankTextNode->setID("level-rank-label"_spr);
        m_mainLayer->addChild(rankTextNode);

        if (auto levelIDLabel = m_mainLayer->getChildByID("cvolton.betterinfo/level-id-label")) {
            if (m_compactView) {
                rankTextNode->setPositionX(levelIDLabel->getPositionX() - levelIDLabel->getScaledContentWidth() - 2.0f);
            }
            else {
                rankTextNode->setPositionY(levelIDLabel->getPositionY() - levelIDLabel->getScaledContentHeight());
            }
        }
    }
};
