// Based on and inspired by Freeverb (Jezar at Dreampoint, public domain).

#include "plugin.hpp"
#include <cmath>

struct Combverb : Module
{
    enum ParamId
    {
        FEEDBACK_PARAM,
        DAMP_PARAM,
        MIX_PARAM,
        PARAMS_LEN
    };
    enum InputId
    {
        IN_INPUT,
        FEEDBACK_CV_INPUT,
        DAMP_CV_INPUT,
        MIX_CV_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        OUT_OUTPUT,
        OUTPUTS_LEN
    };

    static constexpr int NUM_COMBS = 8;
    static constexpr int NUM_AP = 2;

    static constexpr int COMB_LEN[8] = {521, 631, 769, 929, 1129, 1373, 1663, 2017};
    static constexpr int AP_LEN[NUM_AP] = {556, 441};
    static constexpr int COMB_MAX = 2017;
    static constexpr int AP_MAX = 556;

    static constexpr float AP_FB = 0.5f;
    static constexpr float INPUT_GAIN = 0.015f;
    static constexpr float FB_CEILING = 4.f;
    static constexpr float DAMP_FC_MAX = 18000.f;
    static constexpr float DAMP_FC_MIN = 200.f;
    static constexpr float TWO_PI = 6.28318530718f;

    static constexpr float UNITY_POS = 2.f / 3.f;
    static constexpr float DECAY_MIN = 0.1f;
    static constexpr float DECAY_MAX = 30.f;
    static constexpr float GROW_MAX = 24.f;
    static constexpr int FB_DIVISION = 16;

    float combBuf[8][COMB_MAX] = {};
    int combIdx[8] = {};
    float combStore[8] = {};

    float apBuf[NUM_AP][AP_MAX] = {};
    int apIdx[NUM_AP] = {};

    float fbCache[8] = {};
    dsp::ClockDivider fbDivider;

    Combverb()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.35f, "Feedback", "%", 0.f, 150.f);
        configParam(DAMP_PARAM, 0.f, 1.f, 0.5f, "Damp");
        configParam(MIX_PARAM, 0.f, 1.f, 0.5f, "Mix");
        configInput(IN_INPUT, "Audio");
        configInput(FEEDBACK_CV_INPUT, "Feedback CV");
        configInput(DAMP_CV_INPUT, "Damp CV");
        configInput(MIX_CV_INPUT, "Mix CV");
        configOutput(OUT_OUTPUT, "Audio");
        fbDivider.setDivision(FB_DIVISION);
    }

    void onSampleRateChange(const SampleRateChangeEvent &e) override
    {
        updateFeedback(params[FEEDBACK_PARAM].getValue(), e.sampleRate);
    }

    void onReset() override
    {
        for (int n = 0; n < 8; n++)
        {
            std::fill(combBuf[n], combBuf[n] + COMB_MAX, 0.f);
            combIdx[n] = 0;
            combStore[n] = 0.f;
        }
        for (int n = 0; n < NUM_AP; n++)
        {
            std::fill(apBuf[n], apBuf[n] + AP_MAX, 0.f);
            apIdx[n] = 0;
        }
    }

    void updateFeedback(float knob, float sampleRate)
    {
        float rate;
        if (knob < UNITY_POS)
            rate = -60.f / (DECAY_MIN * std::pow(DECAY_MAX / DECAY_MIN, knob / UNITY_POS));
        else
            rate = GROW_MAX * (knob - UNITY_POS) / (1.f - UNITY_POS);

        for (int n = 0; n < NUM_COMBS; n++)
            fbCache[n] = std::pow(10.f, rate * COMB_LEN[n] / sampleRate / 20.f);
    }

    float dampCoeff(float knob, float sampleRate)
    {
        float fc = DAMP_FC_MAX * std::pow(DAMP_FC_MIN / DAMP_FC_MAX, knob);
        return std::exp(-TWO_PI * fc / sampleRate);
    }

    float comb(int n, float in, float fb, float damp)
    {
        float out = combBuf[n][combIdx[n]];
        combStore[n] = out * (1.f - damp) + combStore[n] * damp + 1e-20f;
        combBuf[n][combIdx[n]] = FB_CEILING * std::tanh((in + combStore[n] * fb) / FB_CEILING);
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
        float feedback = clamp(params[FEEDBACK_PARAM].getValue()
                           + inputs[FEEDBACK_CV_INPUT].getVoltage() * 0.1f, 0.f, 1.f);
        float damp = dampCoeff(clamp(params[DAMP_PARAM].getValue()
                           + inputs[DAMP_CV_INPUT].getVoltage() * 0.1f, 0.f, 1.f), args.sampleRate);
        float mix = clamp(params[MIX_PARAM].getValue()
                          + inputs[MIX_CV_INPUT].getVoltage() * 0.1f, 0.f, 1.f);

        if (fbDivider.process())
            updateFeedback(feedback, args.sampleRate);

        float wet = 0.f;
        for (int n = 0; n < NUM_COMBS; n++)
            wet += comb(n, in * INPUT_GAIN, fbCache[n], damp);
        for (int n = 0; n < NUM_AP; n++)
            wet = allpass(n, wet);

        outputs[OUT_OUTPUT].setVoltage(clamp(in * (1.f - mix) + wet * mix, -10.f, 10.f));
    }
};

constexpr int Combverb::COMB_LEN[8];
constexpr int Combverb::AP_LEN[Combverb::NUM_AP];


struct CombverbWidget : ModuleWidget
{
    CombverbWidget(Combverb *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Combverb.svg")));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 32)), module, Combverb::FEEDBACK_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 32)), module, Combverb::FEEDBACK_CV_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 56)), module, Combverb::DAMP_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 56)), module, Combverb::DAMP_CV_INPUT));

        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10, 80)), module, Combverb::MIX_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(30.64, 80)), module, Combverb::MIX_CV_INPUT));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 108)), module, Combverb::IN_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.64, 108)), module, Combverb::OUT_OUTPUT));
    }
};

Model *modelCombverb = createModel<Combverb, CombverbWidget>("Combverb");
