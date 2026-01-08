#ifndef WINDOW_HPP
#define WINDOW_HPP

#ifdef __cplusplus
extern "C" {
#endif

// Exported variables
extern "C" {
    extern const char windowTitle[];
    extern void* windowPtr;  // Make sure this is accessible
}

// Exported functions
int InitialiseWindow();
void CleanExitWindow();

#ifdef __cplusplus
}
#endif

#endif // WINDOW_HPP