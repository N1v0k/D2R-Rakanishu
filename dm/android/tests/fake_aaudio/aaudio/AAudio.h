

#pragma once
#include <stdint.h>
typedef int32_t aaudio_result_t;
typedef int32_t aaudio_stream_state_t;
typedef int32_t aaudio_data_callback_result_t;
typedef struct AAudioStreamStruct AAudioStream;
typedef struct AAudioStreamBuilderStruct AAudioStreamBuilder;
typedef aaudio_data_callback_result_t (*AAudioStream_dataCallback)(AAudioStream *stream, void *user, void *data, int32_t frames);
typedef void (*AAudioStream_errorCallback)(AAudioStream *stream, void *user, aaudio_result_t error);
enum { AAUDIO_OK = 0, AAUDIO_ERROR_DISCONNECTED = -899, AAUDIO_ERROR_INVALID_STATE = -895, AAUDIO_ERROR_UNAVAILABLE = -886 };
enum { AAUDIO_DIRECTION_OUTPUT = 0, AAUDIO_FORMAT_PCM_I16 = 1, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY = 12, AAUDIO_USAGE_GAME = 14,
       AAUDIO_CONTENT_TYPE_SONIFICATION = 4, AAUDIO_CALLBACK_RESULT_CONTINUE = 0 };
enum { AAUDIO_STREAM_STATE_OPEN = 2, AAUDIO_STREAM_STATE_STARTED = 4, AAUDIO_STREAM_STATE_STOPPED = 10, AAUDIO_STREAM_STATE_DISCONNECTED = 13 };
aaudio_result_t AAudio_createStreamBuilder(AAudioStreamBuilder **b);
void AAudioStreamBuilder_setDirection(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setSampleRate(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setChannelCount(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setFormat(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setPerformanceMode(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setUsage(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setContentType(AAudioStreamBuilder *b, int32_t v);
void AAudioStreamBuilder_setDataCallback(AAudioStreamBuilder *b, AAudioStream_dataCallback cb, void *user);
void AAudioStreamBuilder_setErrorCallback(AAudioStreamBuilder *b, AAudioStream_errorCallback cb, void *user);
aaudio_result_t AAudioStreamBuilder_openStream(AAudioStreamBuilder *b, AAudioStream **s);
aaudio_result_t AAudioStreamBuilder_delete(AAudioStreamBuilder *b);
int32_t AAudioStream_getSampleRate(AAudioStream *s);
int32_t AAudioStream_getChannelCount(AAudioStream *s);
int32_t AAudioStream_getPerformanceMode(AAudioStream *s);
int32_t AAudioStream_getFramesPerBurst(AAudioStream *s);
int32_t AAudioStream_getBufferSizeInFrames(AAudioStream *s);
int32_t AAudioStream_getBufferCapacityInFrames(AAudioStream *s);
aaudio_stream_state_t AAudioStream_getState(AAudioStream *s);
aaudio_result_t AAudioStream_requestStart(AAudioStream *s);
aaudio_result_t AAudioStream_requestStop(AAudioStream *s);
aaudio_result_t AAudioStream_close(AAudioStream *s);
const char *AAudio_convertResultToText(aaudio_result_t r);
