#include "plugin.hpp"
#include <cmath>
#include <sstream>
#include <string>
#include <vector>


struct XenScribe : Module
{
    enum ParamId
    {
        PARAMS_LEN
    };
    enum InputId
    {
        PITCH_INPUT,
        INPUTS_LEN
    };
    enum OutputId
    {
        PITCH_OUTPUT,
        OUTPUTS_LEN
    };

    // Newline-separated cents from the root (which is implicit at 0). The
    // last value is the period. Default is 12-TET so a fresh module is usable.
    std::string scaleText = "100\n200\n300\n400\n500\n600\n700\n800\n900\n1000\n1100\n1200";
    std::vector<float> pitches;
    float period_cents = 1200.f;

    // UI thread sets scaleDirty when the user types; the audio thread re-parses
    // on the next process() so the vector is only mutated from one thread.
    bool scaleDirty = true;
    // Set by dataFromJson so the widget pulls the loaded string into its
    // displayed text on the next draw().
    bool manualSet = true;

    XenScribe()
    {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        configInput(PITCH_INPUT, "Pitch");
        configOutput(PITCH_OUTPUT, "Pitch");
    }

    void parseScale()
    {
        std::vector<float> parsed;
        std::stringstream ss(scaleText);
        std::string token;
        while (std::getline(ss, token))
        {
            try
            {
                parsed.push_back(std::stof(token));
            }
            catch (...)
            {
            }
        }
        if (parsed.size() >= 2 && parsed.back() > 0.f)
        {
            pitches = parsed;
            period_cents = pitches.back();
        }
    }

    void process(const ProcessArgs &args) override
    {
        if (scaleDirty)
        {
            parseScale();
            scaleDirty = false;
        }
        if (pitches.size() < 2 || period_cents <= 0.f)
            return;

        float input_cents = inputs[PITCH_INPUT].getVoltage() * 1200.f;
        float period_idx_f = std::floor(input_cents / period_cents);
        int period_idx = (int)period_idx_f;
        float remainder = input_cents - period_idx_f * period_cents;

        // Implicit root at 0¢ is the initial best; the last pitch equals
        // period_cents and doubles as snap-up-to-next-period.
        float best_pitch = 0.f;
        float best_dist = std::fabs(remainder);
        for (float p : pitches)
        {
            float d = std::fabs(remainder - p);
            if (d < best_dist)
            {
                best_dist = d;
                best_pitch = p;
            }
        }

        float output_cents = period_idx * period_cents + best_pitch;
        outputs[PITCH_OUTPUT].setVoltage(output_cents / 1200.f);
    }

    json_t *dataToJson() override
    {
        json_t *root = json_object();
        json_object_set_new(root, "scale", json_string(scaleText.c_str()));
        return root;
    }

    void dataFromJson(json_t *root) override
    {
        json_t *j = json_object_get(root, "scale");
        if (j)
        {
            scaleText = json_string_value(j);
            scaleDirty = true;
            manualSet = true;
        }
    }
};


struct ScaleTextField : ui::TextField
{
    XenScribe *module = nullptr;

    void draw(const DrawArgs &args) override
    {
        if (module)
        {
            if (module->manualSet)
            {
                setText(module->scaleText);
                module->manualSet = false;
            }
            else if (text != module->scaleText)
            {
                module->scaleText = text;
                module->scaleDirty = true;
            }
        }
        ui::TextField::draw(args);
    }
};


struct XenScribeWidget : ModuleWidget
{
    XenScribeWidget(XenScribe *module)
    {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/XenScribe.svg")));

        auto *tf = createWidget<ScaleTextField>(mm2px(Vec(2, 2)));
        tf->box.size = mm2px(Vec(36.64, 106));
        tf->multiline = true;
        tf->module = module;
        if (module)
            tf->setText(module->scaleText);
        addChild(tf);

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10, 117)), module, XenScribe::PITCH_INPUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(30.64, 117)), module, XenScribe::PITCH_OUTPUT));
    }
};

Model *modelXenScribe = createModel<XenScribe, XenScribeWidget>("XenScribe");
