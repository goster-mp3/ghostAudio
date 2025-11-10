#include "plugin.hpp"
using namespace rack;

struct Wavemaker : Module {
	enum ParamId {
		FREQ_KNOB_PARAM,
		PW_KNOB_PARAM,
		PW_MOD_KNOB_PARAM,
		FM_KNOB_PARAM,
		AM_KNOB_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		V_OCT_INPUT,
		PWM_INPUT,
		FM_INPUT,
		SYNC_INPUT,
		AM_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		SIN_OUTPUT,
		TRI_OUTPUT,
		SAW_OUTPUT,
		SQR_OUTPUT,
		NOISE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		FM_LIGHT,
		SYNC_LIGHT,
		LIGHTS_LEN
	};

	Wavemaker() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(FREQ_KNOB_PARAM, -5.f, 5.f, 0.f, "Frequency", " Hz", 2, dsp::FREQ_C4);
		configParam(PW_KNOB_PARAM, 0.001f, 0.999f, 0.5f, "Pulse width", "%", 0.f, 100.f);
		configParam(PW_MOD_KNOB_PARAM, -1.f, 1.f, 0.f, "Pulse width modulation", "%", 0.f, 100.f);
		configParam(FM_KNOB_PARAM, -1.f, 1.f, 0.f, "Frequency modulation", "%", 0.f, 100.f);
		configParam(AM_KNOB_PARAM, 0.f, 1.f, 0.f, "Amplitude modulation", "%", 0.f, 100.f);
		configInput(V_OCT_INPUT, "Pitch (1V / octave)");
		configInput(PWM_INPUT, "Pulse width modulation");
		configInput(FM_INPUT, "Frequency modulation");
		configInput(SYNC_INPUT, "Sync");
		configInput(AM_INPUT, "Amplitude modulation");
		configOutput(SIN_OUTPUT, "Sine");
		configOutput(TRI_OUTPUT, "Triangle");
		configOutput(SAW_OUTPUT, "Sawtooth");
		configOutput(SQR_OUTPUT, "Square");
		configOutput(NOISE_OUTPUT, "White noise");
	}

	float phase = 0.f;
	dsp::SchmittTrigger syncTrigger;
	float sawFilterInit = 0.f;
	float squareFilterInit = 0.f;
	float syncLight = 0.f;

	void process(const ProcessArgs& args) override {

		float pitch = params[FREQ_KNOB_PARAM].getValue() + inputs[V_OCT_INPUT].getVoltage();
		float freq = std::pow(2.f, pitch) * dsp::FREQ_C4; // pitch to frequency

		// FM

		if (inputs[FM_INPUT].isConnected()) { 
			float fmDepth = params[FM_KNOB_PARAM].getValue();
			float fmSignal = inputs[FM_INPUT].getVoltage();
			freq += fmDepth * fmSignal * dsp::FREQ_C4 / 5.f;
			lights[FM_LIGHT].setBrightness((std::abs(fmDepth)));
		}

		else {
			lights[FM_LIGHT].setBrightness(0.f);
		}
		
		float phaseInc = freq * args.sampleTime;

		// sync

		if (inputs[SYNC_INPUT].isConnected()) {

			if (syncTrigger.process(inputs[SYNC_INPUT].getVoltage(), 0.1f, 1.f)) {
					phase = 0.f; // reset phase
					syncLight = 1.f;
			}
		}

		// sync light fade

		float fadeLight = 0.15f;
		syncLight -= args.sampleTime / fadeLight;
		if (syncLight < 0.f) syncLight = 0.f;

		lights[SYNC_LIGHT].setBrightness(syncLight);

		// waveform generation

		float sine = std::sin(phase * 2.f * M_PI);
		float triangle = 4.f * fabsf(phase - 0.5f) - 1.f;
		float sawtooth = 2.f * phase - 1.f;

		// PWM
		
			float pwm = params[PW_KNOB_PARAM].getValue();

			if (inputs[PWM_INPUT].isConnected()) {

				float pwModSignal = inputs[PWM_INPUT].getVoltage() / 10.f;
				float pwModDepth = params[PW_MOD_KNOB_PARAM].getValue();
				pwm += pwModSignal * pwModDepth * 0.5f;
			}

			pwm = clamp(pwm, 0.01f, 0.99f);


			float square = (phase < pwm) ? 1.f : -1.f;
			float noise = random::uniform() * 2.f - 1.f; // white noise
		
		// AM

		if (inputs[AM_INPUT].isConnected()) {

			float amSignal = inputs[AM_INPUT].getVoltage() / 10.f;
			float amDepth = params[AM_KNOB_PARAM].getValue();
			
			float am = 1.f - amDepth + (amSignal * amDepth);

			sine *= am;
			triangle *= am;
			sawtooth *= am;
			square *= am;
		}

		// outputs + scaling to +/-5V

		outputs[SIN_OUTPUT].setVoltage(sine * 5.f);	
		outputs[TRI_OUTPUT].setVoltage(triangle * 5.f);
		outputs[SAW_OUTPUT].setVoltage(sawFiltered * 5.f);
		outputs[SQR_OUTPUT].setVoltage(squareFiltered * 5.f);
		outputs[NOISE_OUTPUT].setVoltage(noise * 5.f);

		phase += phaseInc;
		if (phase >= 1.f) {
			phase -= 1.f;
		}

			else if (phase < 0.f) {
			phase += 1.f;
			}

	}
};


struct WavemakerWidget : ModuleWidget {
	WavemakerWidget(Wavemaker* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Wavemaker.svg")));

		addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(14.94, 34.87)), module, Wavemaker::FREQ_KNOB_PARAM));
		addParam(createParamCentered<RoundLargeBlackKnob>(mm2px(Vec(14.94, 59.1)), module, Wavemaker::PW_KNOB_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(36.39, 48)), module, Wavemaker::PW_MOD_KNOB_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(9.063, 83)), module, Wavemaker::FM_KNOB_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(41.421, 83)), module, Wavemaker::AM_KNOB_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36.39, 33.46)), module, Wavemaker::V_OCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(36.39, 59.056)), module, Wavemaker::PWM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(9.063, 94.5)), module, Wavemaker::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.297, 94.5)), module, Wavemaker::SYNC_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(41.421, 94.5)), module, Wavemaker::AM_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.04, 111.62)), module, Wavemaker::SIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(16.3, 111.62)), module, Wavemaker::TRI_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.53, 111.62)), module, Wavemaker::SAW_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.648, 111.62)), module, Wavemaker::SQR_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(43.75, 111.62)), module, Wavemaker::NOISE_OUTPUT));

		addChild(createLight<MediumLight<WhiteLight>>(mm2px(Vec(14, 81.62)), module, Wavemaker::FM_LIGHT));
		addChild(createLight<MediumLight<WhiteLight>>(mm2px(Vec(23.83, 81.62)), module, Wavemaker::SYNC_LIGHT));
	}
};

Model* modelWavemaker = createModel<Wavemaker, WavemakerWidget>("Wavemaker");