#pragma once



/*
    Test code for custom UI using declarative layout.
*/
namespace examples
{
struct Small
{
    static constexpr auto name() { return "Small Guy"; }
    static constexpr auto c_name() { return "ngry_small"; }
    static constexpr auto uuid() { return "c07a2fac-ef46-4d2f-b0c6-6b2d17b6b48e"; }

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
                const float max = 1.;
                const float init = 0.5;
            };

            float value;
        } cutoff1, cutoff2;

        struct
        {
            static constexpr auto name() { return "Quality Factor"; }
            enum widget { knob };
            struct range
            {
                const float min = 0.;
                const float max = 1.;
                const float init = 0.5;
            };

            float value;
        } Q1, Q2;

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
        } filter1_toggle, filter2_toggle;
        
    } inputs ;
    
    struct setup 
    { 
        float rate; 
    };

    void prepare(setup s)
    {
        inputs.audio.sampleRate = s.rate;
    }

    struct 
    {
        struct 
        {
        static constexpr auto name() { return "Output"; }
        double** samples;
        int channels;
        } audio;
    } outputs;

    void operator()(int N)
    {
        auto& in = inputs.audio;
        constexpr auto channels = std::decay_t<decltype(in)>::channels();
        auto rate = in.sampleRate;

        auto& out = outputs.audio;

        //Since DSPFilters functions modify the samples in-place, we first copy the input into the output and then process the output samples directly
        for(auto i = 0; i < channels; ++i)
        {
            auto& in_samp  = in.samples[i];
            auto& out_samp = out.samples[i];
            for(auto j = 0; j < N; ++j)
            {
                out_samp[j] = in_samp[j];
            }
        }
    }
    // ********************************************************************************
    //                              UI Part of the object
    // ********************************************************************************
    
    struct ui // defines the main layout
    {
        static constexpr auto name() { return "Main"; }
        static constexpr auto width() { return 500; }
        static constexpr auto height() { return 250; }
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

        struct // Filter 1 layout
        {

            static constexpr auto width() { return 200; }
            static constexpr auto height() { return 150; }
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

            decltype(&ins::cutoff1) cutoff_freq_widget = &ins::cutoff1;
            decltype(&ins::Q1) Q_widget = &ins::Q1;
            decltype(&ins::filter1_toggle) toggle_widget = &ins::filter1_toggle;
        } filter1;

        // struct
        // {
        //     static constexpr auto layout()
        //     {
        //         enum
        //         {
        //             spacing
        //         } d{};
        //         return d;
        //     }
        //     static constexpr auto width() { return 10; }
        //     static constexpr auto height() { return 50; }
        // } spacing1;

        struct // Filter 2 layout
        {
            static constexpr auto width() { return 200; }
            static constexpr auto height() { return 150; }
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

            decltype(&ins::cutoff2) cutoff_freq_widget = &ins::cutoff2;
            decltype(&ins::Q2) Q_widget = &ins::Q2;
            decltype(&ins::filter2_toggle) toggle_widget = &ins::filter2_toggle;
        } filter2;

        // struct
        // {
        //     static constexpr auto layout()
        //     {
        //         enum
        //         {
        //             spacing
        //         } d{};
        //         return d;
        //     }
        //     static constexpr auto width() { return 20; }
        //     static constexpr auto height() { return 20; }
        // } spacing2;
    };
};
}