#pragma once

#include "../ecs/world.hpp"
#include "../ecs/entity.hpp"
#include "../components/dialogue.hpp"
#include "../components/aladdin-controller.hpp"
#include "../components/mesh-renderer.hpp"
#include "../input/keyboard.hpp"
#include "../input/mouse.hpp"

#include <json/json.hpp>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace our {

    /**
     * @brief Drives proximity-triggered conversations between Aladdin and the
     *        Genie.
     *
     * The system owns a small cache of parsed meeting scripts and one shared
     * style sheet. Each frame it:
     *   1. Scans every DialogueTriggerComponent in the world and, if no
     *      conversation is active, starts one when the player enters a
     *      trigger's radius.
     *   2. While a conversation is running it freezes gameplay (the play
     *      state queries @ref isActive()), types out the current line, and
     *      lets the player advance with SPACE / ENTER / LMB — or ESC to skip.
     *   3. When the last line has been advanced past, or the user skips, it
     *      applies the script's on-finish hooks (reveal / remove entities).
     *
     * The system is header-only to match the rest of the engine's conventions.
     */
    class DialogueSystem {
    public:
        /// Runtime styling resolved from config/levels/dialogues/style.json.
        struct SpeakerStyle {
            std::string displayName;
            ImVec4 nameColor   {1.00f, 0.90f, 0.35f, 1.00f};
            ImVec4 textColor   {1.00f, 0.94f, 0.78f, 1.00f};
            ImVec4 background  {0.08f, 0.06f, 0.04f, 0.88f};
            ImVec4 borderColor {0.80f, 0.60f, 0.20f, 1.00f};
        };

        struct Style {
            std::unordered_map<std::string, SpeakerStyle> speakers;
            float charactersPerSecond = 42.0f;
            float paddingX = 24.0f;
            float paddingY = 18.0f;
            float borderThickness = 3.0f;
            float cornerRadius = 10.0f;
            float bottomMargin = 36.0f;
            float maxWidthRatio = 0.78f;
            bool  loaded = false;
        };

        struct Line {
            std::string speaker;
            std::string text;
        };

        struct Script {
            std::string title;
            std::vector<Line> lines;
            bool loaded = false;
        };

        bool isActive() const { return active; }

        /// Reset transient state (call when (re)entering the play state).
        void reset() {
            active = false;
            activeTrigger = nullptr;
            activeRevealEntity = nullptr;
            currentLine = 0;
            visibleChars = 0.0f;
            finished = false;
            hidPreReveal = false;
            activeScript = {};
        }

        /// Main tick — proximity check, input handling, typing animation.
        void update(World* world, const Keyboard& keyboard, const Mouse& mouse,
                    const glm::vec3& playerPos, float deltaTime)
        {
            if (!style.loaded) loadStyle("config/levels/dialogues/style.json");

            hideDialogueNPCsOnce(world);

            if (!active) {
                tryTriggerNearbyDialogue(world, playerPos);
                return;
            }

            // ESC always exits the meeting (as the user specified).
            if (keyboard.justPressed(GLFW_KEY_ESCAPE)) {
                finish(world);
                return;
            }

            const bool advance =
                keyboard.justPressed(GLFW_KEY_SPACE) ||
                keyboard.justPressed(GLFW_KEY_ENTER) ||
                keyboard.justPressed(GLFW_KEY_E) ||
                mouse.justPressed(GLFW_MOUSE_BUTTON_LEFT);

            const auto& lines = activeScript.lines;
            if (currentLine >= lines.size()) {
                finish(world);
                return;
            }

            const auto& line = lines[currentLine];
            const float fullLen = static_cast<float>(line.text.size());
            const bool stillTyping = visibleChars < fullLen;

            if (advance) {
                if (stillTyping) {
                    // First press during typing reveals the rest of the line.
                    visibleChars = fullLen;
                } else {
                    // Move to next line (or finish if this was the last one).
                    ++currentLine;
                    visibleChars = 0.0f;
                    if (currentLine >= lines.size()) {
                        finish(world);
                        return;
                    }
                }
            } else if (stillTyping) {
                visibleChars = std::min(fullLen, visibleChars + style.charactersPerSecond * deltaTime);
            }
        }

        /// Draw the conversation box. Call from the state's onImmediateGui().
        void renderImGui() const {
            if (!active) return;
            if (currentLine >= activeScript.lines.size()) return;

            ImGuiIO& io = ImGui::GetIO();
            const float screenW = io.DisplaySize.x;
            const float screenH = io.DisplaySize.y;

            const Line& line = activeScript.lines[currentLine];
            const SpeakerStyle& sp = resolveSpeaker(line.speaker);

            const float boxW = screenW * style.maxWidthRatio;
            const float boxH = 160.0f;
            const ImVec2 boxPos(
                (screenW - boxW) * 0.5f,
                screenH - boxH - style.bottomMargin);
            const ImVec2 boxEnd(boxPos.x + boxW, boxPos.y + boxH);

            ImDrawList* dl = ImGui::GetForegroundDrawList();

            // Dim the rest of the screen a touch so the box reads better.
            dl->AddRectFilled(ImVec2(0, 0), ImVec2(screenW, screenH),
                              ImGui::GetColorU32(ImVec4(0, 0, 0, 0.35f)));

            // Box background + border.
            dl->AddRectFilled(boxPos, boxEnd,
                              ImGui::GetColorU32(sp.background),
                              style.cornerRadius);
            dl->AddRect(boxPos, boxEnd,
                        ImGui::GetColorU32(sp.borderColor),
                        style.cornerRadius,
                        0,
                        style.borderThickness);

            // Speaker name (top of the box).
            const ImVec2 namePos(boxPos.x + style.paddingX, boxPos.y + style.paddingY * 0.6f);
            ImFont* font = ImGui::GetFont();
            const float baseSize = font->FontSize;
            dl->AddText(font, baseSize * 1.45f, namePos,
                        ImGui::GetColorU32(sp.nameColor),
                        sp.displayName.c_str());

            // Body text — word-wrapped, typed out up to visibleChars.
            const int shownChars = std::min<int>(static_cast<int>(visibleChars),
                                                 static_cast<int>(line.text.size()));
            const std::string visible = line.text.substr(0, shownChars);

            const ImVec2 textPos(boxPos.x + style.paddingX,
                                 namePos.y + baseSize * 1.45f + 10.0f);
            const float wrapWidth = boxW - style.paddingX * 2.0f;
            dl->AddText(font, baseSize * 1.15f, textPos,
                        ImGui::GetColorU32(sp.textColor),
                        visible.c_str(), nullptr, wrapWidth);

            // Hint at the bottom-right of the box.
            const bool typingDone = visibleChars >= static_cast<float>(line.text.size());
            const char* hint = typingDone
                ? "SPACE / E  continue      ESC  skip"
                : "SPACE / E  reveal       ESC  skip";
            const ImVec2 hintSize = ImGui::CalcTextSize(hint);
            const ImVec2 hintPos(boxEnd.x - style.paddingX - hintSize.x,
                                 boxEnd.y - style.paddingY - hintSize.y);
            dl->AddText(font, baseSize * 0.95f, hintPos,
                        ImGui::GetColorU32(ImVec4(1, 1, 1, 0.55f)),
                        hint);
        }

    private:
        // --- Style / script loading ---------------------------------------

        void loadStyle(const std::string& path) {
            style.loaded = true; // mark even on failure — we fall back to defaults
            std::ifstream in(path);
            if (!in) {
                std::cerr << "[DialogueSystem] Could not open style file: " << path << "\n";
                fillDefaultStyle();
                return;
            }
            nlohmann::json j;
            try { in >> j; } catch (std::exception& e) {
                std::cerr << "[DialogueSystem] Style JSON parse error: " << e.what() << "\n";
                fillDefaultStyle();
                return;
            }

            if (j.contains("typing") && j["typing"].is_object()) {
                style.charactersPerSecond = j["typing"].value("charactersPerSecond", style.charactersPerSecond);
            }
            if (j.contains("box") && j["box"].is_object()) {
                const auto& b = j["box"];
                style.paddingX = b.value("paddingX", style.paddingX);
                style.paddingY = b.value("paddingY", style.paddingY);
                style.borderThickness = b.value("borderThickness", style.borderThickness);
                style.cornerRadius = b.value("cornerRadius", style.cornerRadius);
                style.bottomMargin = b.value("bottomMargin", style.bottomMargin);
                style.maxWidthRatio = b.value("maxWidthRatio", style.maxWidthRatio);
            }
            if (j.contains("speakers") && j["speakers"].is_object()) {
                for (auto it = j["speakers"].begin(); it != j["speakers"].end(); ++it) {
                    SpeakerStyle sp;
                    const auto& v = it.value();
                    sp.displayName = v.value("displayName", it.key());
                    readColor(v, "nameColor",   sp.nameColor);
                    readColor(v, "textColor",   sp.textColor);
                    readColor(v, "background",  sp.background);
                    readColor(v, "borderColor", sp.borderColor);
                    style.speakers[toLower(it.key())] = sp;
                }
            }
            if (style.speakers.empty()) fillDefaultStyle();
        }

        Script loadScript(const std::string& path) const {
            Script s;
            std::ifstream in(path);
            if (!in) {
                std::cerr << "[DialogueSystem] Could not open dialogue script: " << path << "\n";
                return s;
            }
            nlohmann::json j;
            try { in >> j; } catch (std::exception& e) {
                std::cerr << "[DialogueSystem] Script JSON parse error: " << e.what() << "\n";
                return s;
            }
            s.title = j.value("title", std::string(""));
            if (j.contains("lines") && j["lines"].is_array()) {
                for (const auto& l : j["lines"]) {
                    if (!l.is_object()) continue;
                    Line line;
                    line.speaker = l.value("speaker", std::string("aladdin"));
                    line.text    = l.value("text", std::string(""));
                    if (!line.text.empty()) s.lines.push_back(std::move(line));
                }
            }
            s.loaded = true;
            return s;
        }

        // --- Trigger / state management -----------------------------------

        void hideDialogueNPCsOnce(World* world) {
            // One-shot: the first time we see each trigger with a revealEntity,
            // force the referenced entity's mesh renderer to be hidden. This
            // lets the level config stay minimal — no need to repeat
            // "visible: false" manually for every NPC.
            if (hidPreReveal || !world) return;
            hidPreReveal = true;

            for (auto e : world->getEntities()) {
                auto* trig = e->getComponent<DialogueTriggerComponent>();
                if (!trig) continue;
                if (trig->revealEntity.empty()) continue;
                if (auto* target = findEntityByName(world, trig->revealEntity)) {
                    if (auto* mr = target->getComponent<MeshRendererComponent>()) {
                        mr->visible = false;
                    }
                }
            }
        }

        void tryTriggerNearbyDialogue(World* world, const glm::vec3& playerPos) {
            for (auto e : world->getEntities()) {
                auto* trig = e->getComponent<DialogueTriggerComponent>();
                if (!trig) continue;
                if (trig->fired && trig->oneShot) continue;

                const glm::vec3 triggerPos = glm::vec3(
                    e->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1));
                if (glm::distance(playerPos, triggerPos) > trig->radius) continue;

                startDialogue(world, e, trig);
                return;
            }
        }

        void startDialogue(World* world, Entity* triggerEntity, DialogueTriggerComponent* trig) {
            activeScript = loadScript(trig->scriptPath);
            if (!activeScript.loaded || activeScript.lines.empty()) {
                // Failed to load — mark fired so we don't spam-retry every frame.
                trig->fired = true;
                return;
            }

            active = true;
            activeTrigger = triggerEntity;
            activeRevealEntity = nullptr;
            currentLine = 0;
            visibleChars = 0.0f;
            finished = false;
            trig->fired = true;

            if (!trig->revealEntity.empty()) {
                if (auto* target = findEntityByName(world, trig->revealEntity)) {
                    activeRevealEntity = target;
                    if (auto* mr = target->getComponent<MeshRendererComponent>()) {
                        mr->visible = true;
                    }
                }
            }

            std::cout << "[DialogueSystem] Started: " << activeScript.title
                      << " (" << activeScript.lines.size() << " lines)\n";
        }

        void finish(World* world) {
            if (!active) return;

            // Pull the on-finish hooks off the trigger before we touch the
            // world — they tell us which entities to remove / reveal.
            DialogueTriggerComponent* trig = activeTrigger
                ? activeTrigger->getComponent<DialogueTriggerComponent>()
                : nullptr;

            if (trig) {
                for (const auto& name : trig->revealEntitiesOnFinish) {
                    if (auto* e = findEntityByName(world, name)) {
                        if (auto* mr = e->getComponent<MeshRendererComponent>()) {
                            mr->visible = true;
                        }
                    }
                }
                for (const auto& name : trig->removeEntitiesOnFinish) {
                    if (auto* e = findEntityByName(world, name)) {
                        if (auto* mr = e->getComponent<MeshRendererComponent>()) {
                            mr->visible = false; // hide immediately this frame
                        }
                        world->markForRemoval(e);
                    }
                }
            }
            if (activeRevealEntity) {
                // Genie leaves at end of meeting too, unless explicitly kept.
                if (auto* mr = activeRevealEntity->getComponent<MeshRendererComponent>()) {
                    mr->visible = false;
                }
                world->markForRemoval(activeRevealEntity);
            }

            std::cout << "[DialogueSystem] Finished: " << activeScript.title << "\n";

            activeScript = {};
            active = false;
            activeTrigger = nullptr;
            activeRevealEntity = nullptr;
            currentLine = 0;
            visibleChars = 0.0f;
            finished = true;
        }

        // --- Helpers ------------------------------------------------------

        static Entity* findEntityByName(World* world, const std::string& name) {
            if (!world || name.empty()) return nullptr;
            for (auto e : world->getEntities()) {
                if (e->name == name) return e;
            }
            return nullptr;
        }

        static std::string toLower(std::string s) {
            for (auto& c : s) c = static_cast<char>(std::tolower(c));
            return s;
        }

        static void readColor(const nlohmann::json& obj, const char* key, ImVec4& out) {
            if (!obj.contains(key)) return;
            const auto& v = obj[key];
            if (!v.is_array() || v.size() < 3) return;
            out.x = v[0].get<float>();
            out.y = v[1].get<float>();
            out.z = v[2].get<float>();
            out.w = v.size() >= 4 ? v[3].get<float>() : 1.0f;
        }

        const SpeakerStyle& resolveSpeaker(const std::string& speaker) const {
            auto it = style.speakers.find(toLower(speaker));
            if (it != style.speakers.end()) return it->second;
            // Fall back to the first speaker, or a hard-coded default.
            if (!style.speakers.empty()) return style.speakers.begin()->second;
            static const SpeakerStyle fallback{};
            return fallback;
        }

        void fillDefaultStyle() {
            SpeakerStyle al;
            al.displayName  = "Aladdin";
            al.nameColor    = ImVec4(1.00f, 0.78f, 0.35f, 1.00f);
            al.textColor    = ImVec4(1.00f, 0.94f, 0.78f, 1.00f);
            al.background   = ImVec4(0.18f, 0.10f, 0.04f, 0.88f);
            al.borderColor  = ImVec4(0.85f, 0.62f, 0.22f, 1.00f);
            style.speakers["aladdin"] = al;

            SpeakerStyle gn;
            gn.displayName  = "Genie";
            gn.nameColor    = ImVec4(0.55f, 0.92f, 1.00f, 1.00f);
            gn.textColor    = ImVec4(0.80f, 0.96f, 1.00f, 1.00f);
            gn.background   = ImVec4(0.04f, 0.10f, 0.24f, 0.88f);
            gn.borderColor  = ImVec4(0.45f, 0.82f, 1.00f, 1.00f);
            style.speakers["genie"] = gn;
        }

        // --- State --------------------------------------------------------

        Style    style;
        Script   activeScript;
        bool     active = false;
        bool     hidPreReveal = false;
        bool     finished = false;

        Entity*  activeTrigger = nullptr;
        Entity*  activeRevealEntity = nullptr;
        std::size_t currentLine = 0;
        float    visibleChars = 0.0f;
    };

}
