#include <jni.h>
#include <android/log.h>
#include <math.h>

#undef com_jackz314_resample_Resample_DEFAULT_BUFFER_SIZE
#define com_jackz314_resample_Resample_DEFAULT_BUFFER_SIZE 4096L
#undef com_jackz314_resample_Resample_MAX_CHANNELS
#define com_jackz314_resample_Resample_MAX_CHANNELS 1L
#undef com_jackz314_resample_Resample_CHANNEL_MONO
#define com_jackz314_resample_Resample_CHANNEL_MONO 0L
#undef com_jackz314_resample_Resample_CHANNEL_LEFT
#define com_jackz314_resample_Resample_CHANNEL_LEFT 0L
#undef com_jackz314_resample_Resample_CHANNEL_RIGHT
#define com_jackz314_resample_Resample_CHANNEL_RIGHT 1L

#include "resample.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

typedef struct {
    int num_channels;
    struct rs_data **rs;
} resample_data;

JNIEXPORT jdouble JNICALL
Java_com_jackz314_resample_Resample_getFactor__J(JNIEnv *env, jobject instance, jlong ptr) {
    resample_data* data = (resample_data *) ptr;
    return data->rs[com_jackz314_resample_Resample_CHANNEL_MONO]->factor;
}

JNIEXPORT jlong JNICALL
Java_com_jackz314_resample_Resample_init__IIII(JNIEnv *env, jobject instance, jint inputRate,
                                                  jint outputRate, jint bufferSize, jint channels) {
    resample_data* data = malloc(sizeof(resample_data));
    int i;
    data->num_channels = channels;
    if (data->num_channels > com_jackz314_resample_Resample_MAX_CHANNELS) {
        __android_log_print(ANDROID_LOG_DEBUG, "libresample.so",
                            "Resample supports stereo & mono only!");
        return -1;
    }

    data->rs = calloc(data->num_channels, sizeof(struct rs_data *));
    for (i = 0; i < data->num_channels; i++) {
        data->rs[i] =
                resample_init(inputRate, outputRate, bufferSize);
    }

    return (jlong) data;

}

JNIEXPORT void JNICALL
Java_com_jackz314_resample_Resample_close(JNIEnv *env, jobject instance, jlong ptr) {

    resample_data* data = (resample_data *) ptr;
    int i;
    if (data->rs) {
        for (i = 0; i < data->num_channels; i++) {
            resample_close(data->rs[i]);
        }
        free(data->rs);
        data->rs = NULL;
        data->num_channels = 0;
        free(data);
    }
}

JNIEXPORT jint JNICALL
Java_com_jackz314_resample_Resample_resample__DLjava_nio_ByteBuffer_2Ljava_nio_ByteBuffer_2I(
        JNIEnv *env, jobject instance, jdouble factor, jobject inputBuffer, jobject outputBuffer,
        jint byteLen) {
    const char* inData = (char *) (*env)->GetDirectBufferAddress(env, inputBuffer);
    const char* outData = (char *) (*env)->GetDirectBufferAddress(env, outputBuffer);
    int scale = sizeof(short) / sizeof(char);
    int shortLen = byteLen / scale;
    int num = resample_simple(factor, (short *) inData, (short *) outData, shortLen);
    return num * scale;
}

JNIEXPORT jint JNICALL
Java_com_jackz314_resample_Resample_resampleEx(JNIEnv *env, jobject instance, jlong ptr,
                                               jobject inputBuffer, jobject outputBuffer,
                                               jint byteLen) {
    int res;
    resample_data* data = (resample_data *) ptr;
    const char* inData = (char *) (*env)->GetDirectBufferAddress(env, inputBuffer);
    const char* outData = (char *) (*env)->GetDirectBufferAddress(env, outputBuffer);
    int scale = sizeof(short) / sizeof(char);
    int shortLenIn = byteLen / scale;
    int shortLenOut = (int) ceil(byteLen * 1.0f * data->rs[com_jackz314_resample_Resample_CHANNEL_MONO]->factor / scale);
    res = resample(data->rs[com_jackz314_resample_Resample_CHANNEL_MONO], (short *) inData, shortLenIn, (short *) outData, shortLenOut, 1);
    return res > 0 ? res * scale : -1;
}