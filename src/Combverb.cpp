#include "plugin.hpp"
#include <cmath>

struct Combverb : Module
{
    enum ParamId
    {
        SIZE_PARAM,
        DAMP_PARAM,
        MIX_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        IN_INPUT,
        SIZE_CV_INPUT,
        DAMP_CV_INPUT,
        MIX_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };

    static constexpr int COMB_LEN[8] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static constexpr int AP_LEN[4] = {556, 441, 341, 225};
    static constexpr int COMB_MAX = 1617;
    static constexpr int AP_MAX = 556;

    static constexpr float AP_FB = 0.5f;
    static constexpr float INPUT_GAIN = 0.015f;
    static constexpr float DAMP_SCALE = 0.4f;

    float combBuf[8][COMB_MAX] = {};
    int combIdx[8] = {};
    float combStore[8] = {};

    float apBuf[4][AP_MAX] = {};
    int apIdx[4] = {};

    Combverb()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        configParam(SIZE_PARAM, 0.f, 1.f, 0.5f, "Size");
        configParam(DAMP_PARAM, 0.f, 1.f, 0.5f, "Damp");
        configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix");
        configInput(IN_INPUT, "Audio");
        configInput(SIZE_CV_INPUT, "Size CV");
        configInput(DAMP_CV_INPUT, "Damp CV");
        configInput(MIX_CV_INPUT, "Mix CV");
        configOutput(OUT_OUTPUT, "Audio");
    }

    void onReset() override
    {
        for (int n = 0; n < 8; n++)
        {
            std::fill(combBuf[n], combBuf[n] + COMB_MAX, 0.f);
            combIdx[n] = 0;
            combStore[n] = 0.f;
        }
        for (int n = 0; n < 4; n++)
        {
            std::fill(apBuf[n], apBuf[n] + AP_MAX, 0.f);
            apIdx[n] = 0;
        }
    }

    float combFeedback(float size)
    {
        if (size < 0.7f)
            return 0.7f + size / 0.7f * 0.3f;
        return 1.f + (size - 0.7f) / 0.3f * 0.017f;
    }

    float comb(int n, float in, float fb, float damp)
    {
        float out = combBuf[n][combIdx[n]];
        combStore[n] = out * (1.f - damp) + combStore[n] * damp + 1e-20f;
        combBuf[n][combIdx[n]] = in + combStore[n] * fb;
        if (++combIdx[n] >= COMB_LEN[n])
            combIdx[n] = 0;
        return out;
    }

    float allpass(int n, float in)
    {
        float bufout = apBuf[n][apIdx[n]];
        apBuf[n][apIdx[n]] = in + bufout * AP_FB;
        if (++apIdx[n] >= AP_LEN[n])
            apIdx[n] = 0;
        return -in + bufout;
    }

    void process(const ProcessArgs &args) override
    {
        float in = inputs[IN_INPUT].getVoltage();
        float size = clamp(params[SIZE_PARAM].getValue()
                           + inputs[SIZE_CV_INPUT].getVoltage() * 0.1f, 0.f, 1.f);
        float damp = clamp(params[DAMP_PARAM].getValue()
                           + inputs[DAMP_CV_INPUT].getVoltage() * 0.1f, 0.f, 1.f) * DAMP_SCALE;
        float mix = clamp(params[MIX_PARAM].getValue()
                          + inputs[MIX_CV_INPUT].getVoltage() * 0.1f, 0.f, 1.f);

        float fb = combFeedback(size);

        float wet = 0.f;
        for (int n = 0; n < 8; n++)
            wet += comb(n, in * INPUT_GAIN, fb, damp);
        for (int n = 0; n < 4; n++)
            wet = allpass(n, wet);

        outputs[OUT_OUTPUT].setVoltage(clamp(in * (1.f - mix) + wet * mix, -10.f, 10.f));
    }
};

constexpr int Combverb::COMB_LEN[8];
constexpr int Combverb::AP_LEN[4];


struct CombverbWidget : ModuleWidget
{
    CombverbWidget(Combverb *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Combverb.svg")));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 32)), module, Combverb::SIZE_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 32)), module, Combverb::SIZE_CV_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 56)), module, Combverb::DAMP_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 56)), module, Combverb::DAMP_CV_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 80)), module, Combverb::MIX_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 80)), module, Combverb::MIX_CV_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 108)), module, Combverb::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.64, 108)), module, Combverb::OUT_OUTPUT));
    }
};

Model *modelCombverb = createModel<Combverb, CombverbWidget>("Combverb");
