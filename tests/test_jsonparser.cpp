// Unit tests for jsonParser, driven by synthetic JSON strings via the new
// ReloadFromString() entry point — no files, no hardware.
#include "doctest.h"          // main() provided by test_devicedetect.cpp

#include "jsonParser.h"

TEST_SUITE("json") {
    TEST_CASE("DEVICE block: type / name / timeout / executable") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        bool ok = jp.ReloadFromString(R"({
            "DEVICE": {"type":"midi","name":"Test Pad","timeout":250},
            "header": {"identifier":{"executable":{"exec":"/bin/conMIDI"}}},
            "body": {"modes":[]}
        })", &modes, &header);

        CHECK(ok);
        CHECK(jp.GetLoaded());
        CHECK(jp.GetType() == devType::midi);
        CHECK(jp.DevName == "Test Pad");
        CHECK(jp.GetTimeOut() == 250);
        CHECK(jp.GetHasExec());
        CHECK(jp.Ex.exec == "/bin/conMIDI");
    }

    TEST_CASE("body action: midi input + output, default mode is normal") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        bool ok = jp.ReloadFromString(R"({
            "DEVICE": {"type":"midi","name":"x"},
            "body": {"modes":[{"id":0,"active":true,"actions":[
                {"input":{"type":"midi","b0":144,"b1":10,"b2":100},
                 "output":[{"type":"midi","b0":144,"b1":10,"b2":127}]}
            ]}]}
        })", &modes, &header);

        REQUIRE(ok);
        REQUIRE(modes.size() == 1);
        CHECK(modes[0].index == 0);
        CHECK(modes[0].is_active);
        REQUIRE(modes[0].body_actions.size() == 1);

        const Actions &a = modes[0].body_actions[0];
        CHECK(a.in.tp == midi);
        CHECK(a.in.mAct.midi_mode == midi_normal);
        CHECK((unsigned char)a.in.mAct.midi.byte[0] == 144);
        CHECK((unsigned char)a.in.mAct.midi.byte[1] == 10);
        CHECK((unsigned char)a.in.mAct.midi.byte[2] == 100);
        REQUIRE(a.out.size() == 1);
        CHECK(a.out[0].tp == midi);
        CHECK((unsigned char)a.out[0].mAct.midi.byte[2] == 127);
    }

    TEST_CASE("midi mode strings map to enums (incl. blink, newly supported)") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        bool ok = jp.ReloadFromString(R"({
            "DEVICE": {"type":"midi","name":"x"},
            "body": {"modes":[{"id":0,"active":true,"actions":[
                {"input":{"type":"midi","b0":1,"b1":1,"b2":1,"mode":"trigger_higher"},"output":[]},
                {"input":{"type":"midi","b0":1,"b1":2,"b2":1,"mode":"spot"},"output":[]},
                {"input":{"type":"midi","b0":1,"b1":3,"b2":1,"mode":"blink"},"output":[]}
            ]}]}
        })", &modes, &header);

        REQUIRE(ok);
        REQUIRE(modes[0].body_actions.size() == 3);
        CHECK(modes[0].body_actions[0].in.mAct.midi_mode == midi_trigger_higher);
        CHECK(modes[0].body_actions[1].in.mAct.midi_mode == midi_spot);
        CHECK(modes[0].body_actions[2].in.mAct.midi_mode == midi_blink);
    }

    TEST_CASE("change_mode: enable + change_to parsed") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        bool ok = jp.ReloadFromString(R"({
            "DEVICE": {"type":"midi","name":"x"},
            "body": {"modes":[{"id":0,"active":true,"actions":[
                {"input":{"type":"midi","b0":1,"b1":1,"b2":1},"output":[],
                 "change_mode":{"enable":true,"change_to":2}}
            ]}]}
        })", &modes, &header);

        REQUIRE(ok);
        const Actions &a = modes[0].body_actions[0];
        CHECK(a.change_mode);
        CHECK(a.change_to == 2);
    }

    TEST_CASE("keyboard output: data / keyType / hold / delay") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        bool ok = jp.ReloadFromString(R"({
            "DEVICE": {"type":"midi","name":"x"},
            "body": {"modes":[{"id":0,"active":true,"actions":[
                {"input":{"type":"midi","b0":1,"b1":1,"b2":1},
                 "output":[{"type":"keyboard","data":"letter_a","keyType":"text","hold":"hold","delay":50}]}
            ]}]}
        })", &modes, &header);

        REQUIRE(ok);
        const devActions &out = modes[0].body_actions[0].out[0];
        CHECK(out.tp == keyboard);
        CHECK(out.kData.data == "letter_a");
        CHECK(out.kData.type == text);
        CHECK(out.kData.hold == hold);
        CHECK(out.kData.delay == 50);
    }

    TEST_CASE("joystick input: symbolic code + mode resolve into evtrig") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        bool ok = jp.ReloadFromString(R"({
            "DEVICE": {"type":"joystick","name":"pad"},
            "body": {"modes":[{"id":0,"active":true,"actions":[
                {"input":{"type":"joystick","code":"BTN_SOUTH","mode":"press"},
                 "output":[{"type":"keyboard","data":"letter_a","keyType":"oneKey"}]},
                {"input":{"type":"joystick","code":"ABS_X","mode":"higher","value":200},"output":[]},
                {"input":{"type":"joystick","code":"BTN_START","mode":"hold_once","delay":500},"output":[]},
                {"input":{"type":"joystick","code":"BTN_TL","mode":"hold","interval":100},"output":[]}
            ]}]}
        })", &modes, &header);

        REQUIRE(ok);
        REQUIRE(modes[0].body_actions.size() == 4);

        const devActions &btn = modes[0].body_actions[0].in;
        CHECK(btn.tp == joystick);
        CHECK(btn.evtrig.type == evmatch::EV_KEY_);
        CHECK(btn.evtrig.code == 0x130);             // BTN_SOUTH
        CHECK(btn.evtrig.mode == evmatch::ev_press);

        const devActions &axis = modes[0].body_actions[1].in;
        CHECK(axis.evtrig.type == evmatch::EV_ABS_);
        CHECK(axis.evtrig.code == 0);                // ABS_X
        CHECK(axis.evtrig.mode == evmatch::ev_higher);
        CHECK(axis.evtrig.threshold == 200);

        const devActions &once = modes[0].body_actions[2].in;
        CHECK(once.evtrig.mode == evmatch::ev_hold_once);
        CHECK(once.evtrig.holdMs == 500);            // from "delay"

        const devActions &rep = modes[0].body_actions[3].in;
        CHECK(rep.evtrig.mode == evmatch::ev_hold);
        CHECK(rep.evtrig.holdMs == 100);             // from "interval"
    }

    TEST_CASE("invalid JSON fails gracefully (not loaded)") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        CHECK_FALSE(jp.ReloadFromString("{ this is not json", &modes, &header));
        CHECK_FALSE(jp.GetLoaded());
    }

    TEST_CASE("valid JSON without a DEVICE block is not loaded") {
        std::vector<ModeType> modes; std::vector<Actions> header;
        jsonParser jp("", &modes, &header);
        CHECK_FALSE(jp.ReloadFromString(R"({"header":{}})", &modes, &header));
    }
}
