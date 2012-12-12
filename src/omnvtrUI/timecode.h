#ifndef TIMECODE_H
#define TIMECODE_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/**
 * Convert frames to timecode string (PAL)
 *
 * @param[in] frames number of frames
 * @param[in] buf buffer where to store resulting value
 *
 * @return pointer to sumbitted buffer
 */
char* tc_frames2txt(int frames, char* buf);

/**
 * Parse text presentation of timecode
 *
 * @param[in] buf buffer where to store resulting value
 *
 * @return pointer to sumbitted buffer
 */
long tc_txt2frames(char* buf);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* TIMECODE_H */
