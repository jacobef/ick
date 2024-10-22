#ifndef PREPROCESSOR_DETECTOR_H
#define PREPROCESSOR_DETECTOR_H

enum detection_status {
    IMPOSSIBLE, // The token is invalid, and can't be made valid by the addition of characters at the end
    INCOMPLETE, // The token is invalid, but could be made valid by the addition of characters at the end
    MATCH // The token is valid
};

#endif // PREPROCESSOR_DETECTOR_H
