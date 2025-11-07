# Wavemaker — Multi-Waveform Oscillator Module for VCV Rack 2

**Wavemaker** is a versatile audio oscillator module for VCV Rack 2 designed to **generate multiple waveforms** simultaneously, including sine, triangle, sawtooth, square (with pulse width control), and white noise.

It features **frequency control**, **frequency modulation (FM)**, **amplitude modulation (AM)**, **pulse width modulation (PWM)**, and **hard synchronization (Sync)**, along with an *adaptive filter* that reduces aliasing while still preserving high harmonic information.

It offers both *sound design versatility* and a *clean, efficient DSP architecture* for any modular setup.

This manual provides an overview of all controls, inputs, and outputs, as well as a detailed explanation of the module’s main features and internal behavior.

# 2. Controls and Connections
 
## 2.1 Panel Controls

- **FREQ *(Knob):*** Adjusts the base frequency of the oscillator. Turn clockwise to increase pitch and counterclockwise to decrease pitch.

- **PWM *(Knob):*** Controls the *duty cycle* (pulse width) of the square wave. Narrower pulses emphasize higher harmonics.

- **FM *(Knob):*** Sets the depth of frequency modulation applied when a control voltage (CV) is connected to the FM input. **Positive** values *increase* frequency with positive CV, while **negative** values invert the modulation phase, *decreasing* frequency with higher voltages.

- **AM *(Knob):*** Sets the depth of amplitude modulation applied when a CV signal is connected to the AM input.

- **Sync Indicator *(LED):*** Briefly lights up when a synchronization trigger is detected.

## 2.2 Inputs

- **V/Oct Input:** Controls the oscillator’s pitch according to the standard **1V/octave convention**. Each 1V change corresponds to one octave of pitch shift.

- **FM Input:** CV input for frequency modulation. The FM knob controls the modulation depth.

- **AM Input:** CV input for amplitude modulation, applied equally to all generated waveforms.

- **PWM Input:** CV input for external control of the pulse width of the square wave.

- **Sync Input:** Accepts trigger signals to **reset** the oscillator’s phase (hard sync).
 
## 2.3 Outputs

All outputs are normalized to **±5 V**, according to VCV Voltage Standards.

- **Sine Output:** Produces a pure sine wave.

- **Triangle Output:** Produces a triangle wave.

- **Saw Output:** Produces a sawtooth wave with adaptive anti-alias filtering.

- **Square Output:** Produces a pulse wave with PWM and adaptive filtering.

- **Noise Output:** Produces white noise.

# 3. Functional Overview

## 3.1 Frequency and Phase Control

The oscillator’s frequency is determined by the **FREQ knob and the V/Oct input combined**. The module internally converts the *CV input* into a *frequency ratio* according to the exponential 1V/octave standard.

Phase progression is continuously calculated *each audio sample*. When a trigger is received at the Sync input, the internal phase **resets to zero**, ensuring synchronization with external oscillators or clocks.

## 3.2 Modulation Features

- ### Frequency Modulation (FM)

The FM input modulates the oscillator’s frequency. At **low frequencies**, it produces *vibrato-like* effects; at **audio rates**, it can generate complex, *harmonic-rich tones*.

The **FM knob** defines ***how much modulation*** is going to be applied to the oscillator’s base frequency, based on the incoming voltage.

- ### Amplitude Modulation (AM)

The AM input modulates the amplitude of all generated waveforms simultaneously.

With **low-frequency modulation**, this results in *tremolo* effects; at **higher modulation** speeds, it creates *complex harmonic variations*.

- ### Pulse Width Modulation (PWM)

The square wave output supports **both** manual and CV-controlled pulse width adjustment.

Modifying the duty cycle changes the *waveform’s harmonic content* — a key feature for different textures.

## 3.3 Synchronization (Hard Sync)

When the Sync input receives a *trigger* signal, the oscillator’s phase **resets to zero**.

The LED indicator flashes *each time* a sync trigger is received, providing visual feedback of synchronization.

# 4. Adaptive Anti-Aliasing Filter

Wavemaker includes an *internal adaptive low-pass filter* applied to the **sawtooth and square waveforms**, designed to *reduce aliasing* artifacts at high frequencies.

The filter dynamically **adjusts its cutoff frequency** in relation to the *oscillator’s pitch* and the *audio sampling rate*.

This design ensures:

- Preservation of high harmonic content at low and mid frequencies.

- Smooth attenuation of upper harmonics as pitch increases.

- Natural timbral balance across the oscillator’s full range without introducing phase distortion.

- The filter operates entirely **automatically** — no additional user adjustment is needed.

# 5. Support and Updates

The latest source code, documentation, and update notes are available on the **official GitHub repository**.

If you encounter any issues, please **report them** through the *Issues* section on GitHub, specifying your operating system, Rack version, and steps to reproduce the problem.

# 6. Acknowledgments

***Wavemaker*** was developed as both a **creative and academic** project, focused on providing a ***simple, versatile, and efficient*** oscillator design for VCV Rack users.

Thanks to the **open-source VCV Rack community and developers** whose work inspired this module and to **Audioplace Academy** for introducing me to the world of synthesis and inspiring me to contribute to *the engineering behind the art.*
