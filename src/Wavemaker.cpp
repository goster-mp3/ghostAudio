#include "plugin.hpp"
using namespace rack;

// --- PolyBLEP / PolyBLAMP for antialiasing ---

// polyBLEP corrects value discontinuities, used in saw and square.

static float polyBLEP(float t, float dt) {
	if (dt <= 0.f)
		return 0.f;
 
	// discontinuity just after the edge (from -1 to +1)
	if (t < dt) {
		t /= dt;
		return t + t - t * t - 1.f;
	}
	// discontinuity just before the edge (from +1 to -1)
	if (t > 1.f - dt) {
		t = (t - 1.f) / dt;
		return t * t + t + t + 1.f;
	}
	return 0.f;
}

// polyBLAMP corrects slope discontinuities, used in triangle. 

static float polyBLAMP(float t, float dt) {
	if (dt <= 0.f)
		return 0.f;
 
	if (t < dt) {
		t = t / dt - 1.f;
		return -1.f / 3.f * t * t * t;
	}
	if (t > 1.f - dt) {
		t = (t - 1.f) / dt + 1.f;
		return 1.f / 3.f * t * t * t;
	}
	return 0.f;
}

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
	float syncLight = 0.f;

	void process(const ProcessArgs& args) override {

		float pitch = params[FREQ_KNOB_PARAM].getValue() + inputs[V_OCT_INPUT].getVoltage();
		float freq = std::pow(2.f, pitch) * dsp::FREQ_C4; // pitch to frequency

		// FM

		if (inputs[FM_INPUT].isConnected()) { 
			float fmDepth = params[FM_KNOB_PARAM].getValue();
			float fmSignal = inputs[FM_INPUT].getVoltage();
			freq += fmDepth * fmSignal * dsp::FREQ_C4 / 5.f;

			float brightness = std::abs(fmSignal) * std::abs(fmDepth) / 5.f;
   	 		lights[FM_LIGHT].setBrightness(clamp(brightness, 0.f, 1.f));
		}

		else {
			lights[FM_LIGHT].setBrightness(0.f);
		}
		
		float phaseInc = freq * args.sampleTime;
		float dt = clamp(std::abs(phaseInc), 0.f, 1.f);

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

		// AM is calculated once, and applied to all waveforms that are connected.
		// if AM is not connected, amActive is false and am is 1.f (no effect).
		
		bool amActive = inputs[AM_INPUT].isConnected();
		float am = 1.f;
		if (amActive) {

			float amSignal = inputs[AM_INPUT].getVoltage() / 10.f;
			float amDepth = params[AM_KNOB_PARAM].getValue();
			am = 1.f - amDepth + (amSignal * amDepth);
		}

	// waveform generation

		bool sinConnected = outputs[SIN_OUTPUT].isConnected();
		bool triConnected = outputs[TRI_OUTPUT].isConnected();
		bool sawConnected = outputs[SAW_OUTPUT].isConnected();
		bool sqrConnected = outputs[SQR_OUTPUT].isConnected();
		bool noiseConnected = outputs[NOISE_OUTPUT].isConnected();

		// sine
		if (sinConnected) {
			float sine = std::sin(phase * 2.f * M_PI);
			if (amActive) sine *= am;
			outputs[SIN_OUTPUT].setVoltage(sine * 5.f);
		}

		// triangle
	
		if (triConnected) {
			float triangle = 4.f * fabsf(phase - 0.5f) - 1.f;

			// the slope is discontinuous at phase=0 and phase=0.5, which generates aliasing. We correct it with polyBLAMP.

			triangle += 4.f * dt * polyBLAMP(phase, dt);

			float phase2 = phase + 0.5f;
			if (phase2 >= 1.f) phase2 -= 1.f;
			triangle -= 4.f * dt * polyBLAMP(phase2, dt);

			if (amActive) triangle *= am;
			outputs[TRI_OUTPUT].setVoltage(triangle * 5.f);
		}
		
		// sawtooth
		// discontinuity at phase=0. We correct it with polyBLEP.
		if (sawConnected) {
			float sawtooth = 2.f * phase - 1.f;
			sawtooth -= polyBLEP(phase, dt);

			if (amActive) sawtooth *= am;
			outputs[SAW_OUTPUT].setVoltage(sawtooth * 5.f);
		}

		// square
		// discontinuities at phase=0 and phase=pwm, we correct them with two polyBLEP's.

		if (sqrConnected) {
			float pwm = params[PW_KNOB_PARAM].getValue();

			if (inputs[PWM_INPUT].isConnected()) {
				float pwModSignal = inputs[PWM_INPUT].getVoltage() / 10.f;
				float pwModDepth = params[PW_MOD_KNOB_PARAM].getValue();
				pwm += pwModSignal * pwModDepth * 0.5f;
			}
			pwm = clamp(pwm, 0.01f, 0.99f);

			float square = (phase < pwm) ? 1.f : -1.f;

			// at phase = 0
			square += polyBLEP(phase, dt);

			// at phase = pwm
			float t2 = phase - pwm;
			if (t2 < 0.f) t2 += 1.f;
			square -= polyBLEP(t2, dt);

			if (amActive) square *= am;
			outputs[SQR_OUTPUT].setVoltage(square * 5.f);
		}

		
		// white noise
		if (noiseConnected) {
			float noise = random::uniform() * 2.f - 1.f;
			outputs[NOISE_OUTPUT].setVoltage(noise * 5.f);
		}

		
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