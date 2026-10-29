// Samples a supplied curve function into a table for later evaluation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_CUSTOMCURVE_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_CUSTOMCURVE_H

namespace dice {
namespace hfe {
namespace ai {

// Static curve table with no per-instance state. Construction initializes the shared
// table and destruction releases it.
class SCurve
{
public:
    SCurve();                          // - calls init, nothing else
    ~SCurve();
    static void  init();
    static float calculate(float x);   // - the only one with a NaN guard
    static float* curveValues;
};

class JCurve
{
public:
    JCurve();
    ~JCurve();
    static void  init();
    static float calculate(float x);   // - extrapolates above 1
    static float* curveValues;
};

class DecleiningSlopeCurve             // dice's spelling
{
public:
    DecleiningSlopeCurve();
    ~DecleiningSlopeCurve();
    static void  init();
    static float calculate(float x);
    static float* curveValues;
};

// Samples a function on [min, max] every step and reads it back from the table. The ELF
// places it in AIUtils/CustomCurve.cpp. sizeof == 0x18.
class CustomCurve
{
public:
    typedef float (*Function)(float);

    CustomCurve(Function function, float min, float max, float step);
    ~CustomCurve();

    // A soft step rising from about 1 to about 2 around x = 0.5; the fire behaviour samples it.
    static float modifiedSoftStepFunc(float x);

    // The sample at x, or the straight line between the two around it. An x outside [min, max],
    // or one that lands on the last sample when interpolating, is fatal.
    float calculate(float x, bool interpolate) const;

private:
    CustomCurve(const CustomCurve&);
    CustomCurve& operator=(const CustomCurve&);

    // Resamples the table. A step that is not positive is fatal.
    void init();

    /* +0x00 */ Function m_function;
    /* +0x04 */ float* m_values;                     // m_count + 1 samples
    /* +0x08 */ int m_count;
    /* +0x0C */ float m_min;
    /* +0x10 */ float m_max;
    /* +0x14 */ float m_step;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_CUSTOMCURVE_H
