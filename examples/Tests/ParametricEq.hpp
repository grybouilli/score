#pragma once

#include <vector>
#include <string>
#include <DspFilters/Filter.h>
#include <DspFilters/Butterworth.h>
#include <iostream>

namespace examples
{

struct ParametricEq
{
    static constexpr auto name() { return "Parametric Equalizer"; }
    static constexpr auto c_name() { return "ngry_parameq"; }
    static constexpr auto uuid() { return "ec972d12-edba-4a9c-9736-9176efad17e3"; }


    struct ins
    {
        struct 
        {
            static constexpr auto name() { return "Input"; }
            const double** samples;
            static constexpr auto channels() {  return 2; }
            float sampleRate;
        } audio;

        struct 
        {
            static constexpr auto name() { return "Cutoff Frequency"; }
            enum widget { knob };
            struct range
            {
                const float min = 0.;
                const float max = 22000.;
                const float init = 0.;
            };

            float value;
        } lowpassCutoffFreq, highpassCutoffFreq;//, bandpassCutoffFreq ; 

        struct 
        {
            static constexpr auto name() { return "Center Frequency"; }
            enum widget { knob };
            struct range
            {
                const float min = 0.;
                const float max = 22000.;
                const float init = 0.;
            };

            float value;
        } bandpassCenterFreq;

        struct 
        {
            static constexpr auto name() { return "Frequency Band Width"; }
            enum widget { knob };
            struct range
            {
                const float min = 0.;
                const float max = 1000.;
                const float init = 100.;
            };

            float value;
        } bandpassBandWidth;

        struct order
        {
            static constexpr auto name() { return  "Filter Order"; }
            static constexpr auto maxOrder = 4 ;
            struct range
            {
                const int min = 1;
                const int max = maxOrder;
                const int init = 1;
            };
            int value;
        } lowpassOrder, highpassOrder, bandpassOrder;
        // struct 
        // {
        //     static constexpr auto name() { return "Q"; }
        //     enum widget { knob };
        //     struct range
        //     {
        //         const float min = 0.;
        //         const float max = 1.2;
        //         const float init = .6;
        //     };

        //     float value;
        // } lowpassQ, highpassQ, bandpassQ ;

        struct tog
        {
            enum widget { toggle };
            struct range
            {
                bool v1 = false;
                bool v2 = true;
                bool init = false;
            };
            bool value;
            static constexpr auto name() { return "Apply filter"; }
        } lowpassToggle, highpassToggle, bandpassToggle;

    } inputs;   

    struct 
    {
        struct 
        {
        static constexpr auto name() { return "Output"; }
        double** samples;
        int channels;
        } audio;
    } outputs;

    /* ********************************************************************************
        We leave most of the work to the DSP library. There seem to be some issue with this approach.
        Some noises appear in the output. Issue already encountered while working on more basic lowpass filter.
       ******************************************************************************** */

    struct setup 
    { 
        float rate {}; 
    };

    void prepare(setup s)
    {
        inputs.audio.sampleRate = s.rate;
    }

    void operator()(int N)
    {
        auto& in = inputs.audio;
        constexpr auto channels = std::decay_t<decltype(in)>::channels();
        auto rate = in.sampleRate;

        auto& out = outputs.audio;

        
        //  Since DSPFilters functions modify the samples in-place, 
        //  we first copy the input into the output and then process the output samples directly.
        
        for(auto i = 0; i < channels; ++i)
        {
            auto& in_samp  = in.samples[i];
            auto& out_samp = out.samples[i];
            for(auto j = 0; j < N; ++j)
            {
                out_samp[j] = in_samp[j];
            }
        }

        //Lowpass filter work
        if(inputs.lowpassToggle.value)
        {
            constexpr auto maxOrder = std::decay_t<decltype(inputs.lowpassOrder)>::maxOrder;
            Dsp::SimpleFilter<Dsp::Butterworth::LowPass<maxOrder>, channels> lowpass_filter;
            lowpass_filter.setup (  inputs.lowpassOrder.value,      // order value
                                    rate,                           // sample rate
                                    inputs.lowpassCutoffFreq.value);// cutoff frequency

            lowpass_filter.process (N, out.samples);
        }

        //Highpass filter work
        if(inputs.highpassToggle.value)
        {
            constexpr auto maxOrder = std::decay_t<decltype(inputs.highpassOrder)>::maxOrder;
            Dsp::SimpleFilter<Dsp::Butterworth::HighPass<maxOrder>, channels> highpass_filter;
            highpass_filter.setup ( inputs.highpassOrder.value,      // order value
                                    rate,                           // sample rate
                                    inputs.highpassCutoffFreq.value);// cutoff frequency

            highpass_filter.process (N, out.samples);
        }
        //Bandpass filter work
        if(inputs.bandpassToggle.value)
        {
            constexpr auto maxOrder = std::decay_t<decltype(inputs.bandpassOrder)>::maxOrder;
            Dsp::SimpleFilter<Dsp::Butterworth::BandPass<maxOrder>, channels> bandpass_filter;
            bandpass_filter.setup ( inputs.bandpassOrder.value,      // order value
                                    rate,                            // sample rate
                                    inputs.bandpassCenterFreq.value, // center frequency
                                    inputs.bandpassBandWidth.value);

            bandpass_filter.process (N, out.samples);
        }
    }

    // ********************************************************************************
    //                              UI Part of the object
    // ********************************************************************************

    struct ui // defines the main layout
    {
        static constexpr int globalWidth = 600;
        static constexpr int globalHeight = 500;
        static constexpr auto filtersNb = 3.5;

        static constexpr auto name() { return "Main"; }
        static constexpr auto width() { return globalWidth; }
        static constexpr auto height() { return globalHeight; }
        static constexpr auto layout() 
        {
            enum
            {
                hbox //each sub-layout will be placed horizontally in the main layout
            } d{};
            return d;
        }

        static constexpr auto background() //defines color of the background
        {
            enum
            {
                mid
            } d{};
            return d;
        }

        struct
        {
            static constexpr auto layout()
            {
                enum
                {
                    spacing
                } d{};
                return d;
            }
            static constexpr auto width() { return 10; }
            static constexpr auto height() { return 50; }
        } spacing1;

        struct // Lowpass Filter layout
        {
            static constexpr auto width() { return globalWidth/filtersNb; }
            static constexpr auto height() { return globalHeight/3; }
            const char* f1 = "Lowpass";
            static constexpr auto layout()
            {
                enum
                {
                    vbox
                } d{};
                return d;
            }
            static constexpr auto background()
            {
                enum
                {
                    dark
                } d{};
                return d;
            }

            decltype(&ins::lowpassOrder) order_widget = &ins::lowpassOrder;
            decltype(&ins::lowpassCutoffFreq) cutoff_freq_widget = &ins::lowpassCutoffFreq;
            //decltype(&ins::lowpassQ) Q_widget = &ins::lowpassQ;
            decltype(&ins::lowpassToggle) toggle_widget = &ins::lowpassToggle;
        } lowpass;

        struct // Highpass filter layout
        {
            static constexpr auto width() { return globalWidth/filtersNb; }
            static constexpr auto height() { return globalHeight/3; }
            const char* f1 = "Highpass";
            static constexpr auto layout()
            {
                enum
                {
                    vbox
                } d{};
                return d;
            }
            static constexpr auto background()
            {
                enum
                {
                    dark
                } d{};
                return d;
            }
            decltype(&ins::highpassOrder) order_widget = &ins::highpassOrder;
            decltype(&ins::highpassCutoffFreq) cutoff_freq_widget = &ins::highpassCutoffFreq;
            //decltype(&ins::highpassQ) Q_widget = &ins::highpassQ;
            decltype(&ins::highpassToggle) toggle_widget = &ins::highpassToggle;
        } highpass;
        struct // Bandpass Filter layout
        {
            static constexpr auto width() { return globalWidth/filtersNb; }
            static constexpr auto height() { return globalHeight/2; }
            const char* f1 = "Bandpass";
            static constexpr auto layout()
            {
                enum
                {
                    vbox
                } d{};
                return d;
            }
            static constexpr auto background()
            {
                enum
                {
                    dark
                } d{};
                return d;
            }
            decltype(&ins::bandpassOrder) order_widget = &ins::bandpassOrder;
            decltype(&ins::bandpassCenterFreq) center_freq_widget = &ins::bandpassCenterFreq;
            decltype(&ins::bandpassBandWidth) band_width_widget = &ins::bandpassBandWidth;
            //decltype(&ins::bandpassQ) Q_widget = &ins::bandpassQ;
            decltype(&ins::bandpassToggle) toggle_widget = &ins::bandpassToggle;
        } bandpass;
    };

};
}