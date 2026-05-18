#include "plugin.hpp"
#include <cmath>

struct Scale
{
    const char *name;
    float period_cents;
    static constexpr int N = 12;
    // pitches_cents[0] is the implicit root at 0¢; entries 1..11 are the
    // first 11 values from the .scl file. The 12th .scl value is stored
    // separately as period_cents (it coincides with the next period's root).
    float pitches_cents[N];
};

// Source .scl files preserved under `scales/` for reference; these constants
// are the runtime source of truth and must match.
static const Scale SCALES[] = {
    {"PBDE-71", 3465.227865f, {
        0.f,
        1330.033080f, 1469.405985f, 2550.155464f, 2800.928007f,
        2807.090424f, 2814.811339f, 3004.108439f, 3176.429406f,
        3349.468193f, 3410.253434f, 3429.957446f,
    }},
    {"F2 Toxin", 2789.930318f, {
        0.f,
        774.596992f, 1012.280921f, 1200.107382f, 1304.074136f,
        1549.474373f, 1808.647522f, 1837.699726f, 2175.346535f,
        2404.505227f, 2674.353580f, 2731.978186f,
    }},
    {"Dimethylaminobenzene", 4224.291077f, {
        0.f,
        153.130462f, 273.255341f, 808.639513f, 915.852414f,
        1007.822005f, 2025.864574f, 2787.741966f, 3222.868621f,
        3463.339832f, 3778.853250f, 3954.526901f,
    }},
    {"Mycose", 4329.865218f, {
        0.f,
        121.144693f, 354.802401f, 1026.589323f, 1648.074825f,
        2116.574096f, 2638.426614f, 2823.084797f, 2914.826909f,
        3175.671845f, 3184.209395f, 3866.243670f,
    }},
};
static constexpr int NUM_SCALES = sizeof(SCALES) / sizeof(SCALES[0]);


struct Xenizer : Module
{
    enum ParamId
    {
        SCALE_PARAM,
        MODE_PARAM,
        BASE_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        PITCH_INPUT,
        SCALE_CV_INPUT,
        MODE_CV_INPUT,
        BASE_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        PITCH_OUTPUT,
        OUTPUTS_LEN
    };

    Xenizer()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        configSwitch(SCALE_PARAM, 0.f, (float)(NUM_SCALES - 1), 0.f, "Scale", {
            "PBDE-71", "F2 Toxin", "Dimethylaminobenzene", "Mycose",
        });
        configSwitch(MODE_PARAM, 0.f, (float)(Scale::N - 1), 0.f, "Mode", {
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12",
        });
        configParam(BASE_PARAM, -5.f, 5.f, 0.f, "Base pitch", " V");
        configInput(PITCH_INPUT, "Pitch");
        configInput(SCALE_CV_INPUT, "Scale CV");
        configInput(MODE_CV_INPUT, "Mode CV");
        configInput(BASE_CV_INPUT, "Base CV");
        configOutput(PITCH_OUTPUT, "Pitch");
    }

    void process(const ProcessArgs &args) override
    {
        int scaleIdx = clamp((int)(params[SCALE_PARAM].getValue() + inputs[SCALE_CV_INPUT].getVoltage()), 0, NUM_SCALES - 1);
        const Scale &scale = SCALES[scaleIdx];
        int modeIdx = clamp((int)(params[MODE_PARAM].getValue() + inputs[MODE_CV_INPUT].getVoltage()), 0, Scale::N - 1);
        float base_volts = params[BASE_PARAM].getValue() + inputs[BASE_CV_INPUT].getVoltage();

        // Build rotated pitches: same set of scale pitches, but anchored so
        // scale.pitches_cents[modeIdx] becomes the new 0¢ reference. Pitches
        // that wrap past the period get period_cents added back.
        float root_offset = scale.pitches_cents[modeIdx];
        float rotated_pitches[Scale::N];
        for (int i = 0; i < Scale::N; i++)
        {
            float p = scale.pitches_cents[(modeIdx + i) % Scale::N] - root_offset;
            if (p < 0.f) p += scale.period_cents;
            rotated_pitches[i] = p;
        }

        float input_cents = inputs[PITCH_INPUT].getVoltage() * 1200.f;
        float period_idx_f = std::floor(input_cents / scale.period_cents);
        int period_idx = (int)period_idx_f;
        float remainder = input_cents - period_idx_f * scale.period_cents;

        // Nearest-pitch search across the 12 rotated pitches and the period
        // (period equals the next period's root, so snapping up to it is
        // musically correct when the input is near the top of the period).
        float best_pitch = rotated_pitches[0];
        float best_dist = std::fabs(remainder - rotated_pitches[0]);
        for (int i = 1; i < Scale::N; i++)
        {
            float d = std::fabs(remainder - rotated_pitches[i]);
            if (d < best_dist)
            {
                best_dist = d;
                best_pitch = rotated_pitches[i];
            }
        }
        float d_period = std::fabs(remainder - scale.period_cents);
        if (d_period < best_dist)
        {
            best_pitch = scale.period_cents;
        }

        float output_cents = period_idx * scale.period_cents + best_pitch;
        outputs[PITCH_OUTPUT].setVoltage(output_cents / 1200.f + base_volts);
    }
};


struct XenizerWidget : ModuleWidget
{
    XenizerWidget(Xenizer *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Xenizer.svg")));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 32)), module, Xenizer::SCALE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 32)), module, Xenizer::SCALE_CV_INPUT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 56)), module, Xenizer::MODE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 56)), module, Xenizer::MODE_CV_INPUT));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 80)), module, Xenizer::BASE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 80)), module, Xenizer::BASE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 108)), module, Xenizer::PITCH_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.64, 108)), module, Xenizer::PITCH_OUTPUT));
    }
};

Model *modelXenizer = createModel<Xenizer, XenizerWidget>("Xenizer");
