#ifndef ART_METHOD_H
#define ART_METHOD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <jni.h>

#include "logging.h"

/* INFO: ArtMethod offsets computed at runtime */
static jfieldID art_method_field = NULL;
static size_t art_method_size = 0;
static size_t entry_point_offset = 0;
static size_t data_offset = 0;
/* INFO: Since ART introduction, it has been offset */
static size_t access_flags_offset = sizeof(void *);

/* INFO: Cached java.lang.reflect.Method method IDs */
static jmethodID amethod_get_name_mid = NULL;
static jmethodID amethod_get_parameter_types_mid = NULL;
static jmethodID amethod_get_return_type_mid = NULL;

/* INFO: Access flag constants from ART */
#define AMETHOD_ACC_PUBLIC  0x0001
#define AMETHOD_ACC_PRIVATE 0x0002
#define AMETHOD_ACC_STATIC  0x0008
#define AMETHOD_ACC_NATIVE  0x0100

static inline void *amethod_from_reflected_method(JNIEnv *env, jobject method);
static inline void *amethod_get_data(uintptr_t self);

/* INFO: Convert a Java class name to JNI descriptor. */
static inline size_t amethod_name_to_desc(const char *name, char *buf, size_t buf_size) {
  size_t pos = 0;

  /* INFO: Arrays are already in JNI format. We just replace dots with slashes.
             E.g. [Ljava.lang.String; -> [Ljava/lang/String; */
  if (name[0] == '[') {
    size_t len = strlen(name);
    for (size_t i = 0; i < len && pos < buf_size; i++) {
      buf[pos++] = (name[i] == '.') ? '/' : name[i];
    }

    if (pos < buf_size) buf[pos] = '\0';

    return pos;
  }

  /* INFO: For primitives, as they're single-character descriptors, we can just
             do a lookup on the name and copy the descriptor. */
  {
    static struct {
      const char *name;
      const char *descriptor;
    } prim[] = {
      { "byte", "B" },
      { "char", "C" },
      { "double", "D" },
      { "float", "F" },
      { "int", "I" },
      { "long", "J" },
      { "short", "S" },
      { "boolean", "Z" },
      { "void", "V" }
    };
    for (size_t i = 0; i < sizeof(prim) / sizeof(prim[0]); i++) {
      if (strcmp(name, prim[i].name) != 0) continue;

      size_t len = strlen(prim[i].descriptor);
      if (pos + len < buf_size) memcpy(buf + pos, prim[i].descriptor, len);

      return len;
    }
  }

  /* INFO: For objects, we append a "L" prefix and ";" suffix. E.g. java.lang.String -> Ljava/lang/String; */
  if (pos < buf_size) {
    buf[pos++] = 'L';

    size_t len = strlen(name);
    for (size_t i = 0; i < len && pos < buf_size; i++) {
      buf[pos++] = (name[i] == '.') ? '/' : name[i];
    }
  }

  if (pos < buf_size) buf[pos++] = ';';
  if (pos < buf_size) buf[pos] = '\0';

  return pos;
}

/* INFO: Extract JNI signature from a java.lang.reflect.Method. */
static inline bool amethod_get_sig(JNIEnv *env, jobject method, char *sig, size_t buf_size) {
  size_t pos = 0;
  if (pos < buf_size) sig[pos++] = '(';

  /* INFO: Retrieve parameter types and convert to descriptor */
  {
    jobjectArray params = (jobjectArray)(*env)->CallObjectMethod(env, method, amethod_get_parameter_types_mid);
    if (params) {
      jsize n = (*env)->GetArrayLength(env, params);
      for (jsize i = 0; i < n && pos < buf_size; i++) {
        jobject pc = (*env)->GetObjectArrayElement(env, params, i);
        if (!pc) continue;

        jclass pc_class = (*env)->GetObjectClass(env, pc);
        jmethodID get_name = (*env)->GetMethodID(env, pc_class, "getName", "()Ljava/lang/String;");
        (*env)->DeleteLocalRef(env, pc_class);

        jstring name_str = (jstring)(*env)->CallObjectMethod(env, pc, get_name);
        const char *name = (*env)->GetStringUTFChars(env, name_str, NULL);
        if (!name) goto cleanup_loop;

        pos += amethod_name_to_desc(name, sig + pos, buf_size - pos);
        (*env)->ReleaseStringUTFChars(env, name_str, name);

        cleanup_loop:
          (*env)->DeleteLocalRef(env, pc);
          if (name_str) (*env)->DeleteLocalRef(env, name_str);
      }

      (*env)->DeleteLocalRef(env, params);
    }
  }

  if (pos < buf_size) sig[pos++] = ')';

  /* INFO: Retrieve return type and convert to descriptor */
  {
    jobject rc = (*env)->CallObjectMethod(env, method, amethod_get_return_type_mid);
    if (rc) {
      jclass rc_class = (*env)->GetObjectClass(env, rc);
      jmethodID get_name = (*env)->GetMethodID(env, rc_class, "getName", "()Ljava/lang/String;");
      (*env)->DeleteLocalRef(env, rc_class);

      jstring name_str = (jstring)(*env)->CallObjectMethod(env, rc, get_name);
      const char *name = (*env)->GetStringUTFChars(env, name_str, NULL);
      if (name) {
        pos += amethod_name_to_desc(name, sig + pos, buf_size - pos);
        (*env)->ReleaseStringUTFChars(env, name_str, name);
      }

      (*env)->DeleteLocalRef(env, name_str);
      (*env)->DeleteLocalRef(env, rc);
    }
  }

  if (pos < buf_size) sig[pos] = '\0';

  return pos > 2;
}

/* TODO: Broken! Wrong access flags! */
static inline uint32_t amethod_get_access_flags(JNIEnv *env, jobject method) {
  jlong ptr = (*env)->GetLongField(env, method, art_method_field);
  if (!ptr) return 0;

  return *(uint32_t *)(uintptr_t)(ptr + access_flags_offset);
}

static inline bool amethod_is_native(JNIEnv *env, jobject method) {
  return (amethod_get_access_flags(env, method) & AMETHOD_ACC_NATIVE) != 0;
}

static inline bool amethod_is_static(JNIEnv *env, jobject method) {
  return (amethod_get_access_flags(env, method) & AMETHOD_ACC_STATIC) != 0;
}

static inline void *amethod_get_orig(JNIEnv *env, jobject method) {
  if (art_method_field) {
    jlong ptr = (*env)->GetLongField(env, method, art_method_field);
    if (!ptr) return NULL;

    if ((ptr & 1) == 0)
      return amethod_get_data((uintptr_t)ptr);

    return NULL;
  }

  void *art_method = amethod_from_reflected_method(env, method);
  if (!art_method) return NULL;

  return amethod_get_data((uintptr_t)art_method);
}

static inline bool amethod_free_orig(JNIEnv *env, jobject method, void *orig) {
  if (art_method_field) {
    jlong ptr = (*env)->GetLongField(env, method, art_method_field);
    if (!ptr) return false;

    /* INFO: The jmethodID is a pointer to the artMethod, no need to free */
    if ((ptr & 1) == 0)
      return true;
  }

  (*env)->DeleteLocalRef(env, orig);

  return true;
}

static inline char *amethod_get_name(JNIEnv *env, jobject method) {
  jstring name_str = (jstring)(*env)->CallObjectMethod(env, method, amethod_get_name_mid);
  if (!name_str) return NULL;

  const char *name = (*env)->GetStringUTFChars(env, name_str, NULL);
  if (!name) {
    LOGE("Failed to get method name");

    (*env)->DeleteLocalRef(env, name_str);

    return NULL;
  }

  char *dup = strdup(name);
  (*env)->ReleaseStringUTFChars(env, name_str, name);
  (*env)->DeleteLocalRef(env, name_str);

  return dup;
}

static inline bool amethod_find_native(JNIEnv *env, jclass clazz, const char *target_name, void **out_orig, char **out_sig) {
  jclass cc = (*env)->FindClass(env, "java/lang/Class");
  jmethodID get_methods = (*env)->GetMethodID(env, cc, "getDeclaredMethods", "()[Ljava/lang/reflect/Method;");
  (*env)->DeleteLocalRef(env, cc);
  if (!get_methods) return false;

  jobjectArray methods = (jobjectArray)(*env)->CallObjectMethod(env, clazz, get_methods);
  if (!methods) return false;

  bool found = false;
  jsize methods_len = (*env)->GetArrayLength(env, methods);
  for (jsize i = 0; i < methods_len; i++) {
    jobject method = (*env)->GetObjectArrayElement(env, methods, i);
    if (!method) continue;

    if (!amethod_is_native(env, method)) goto cleanup_method;

    char *name = amethod_get_name(env, method);
    if (!name) goto cleanup_method;

    bool match = (strcmp(name, target_name) == 0);
    free(name);
    if (!match) goto cleanup_method;

    found = true;

    if (out_orig)
      *out_orig = amethod_get_orig(env, method);

    if (out_sig) {
      char sig[512];
      if (amethod_get_sig(env, method, sig, sizeof(sig)))
        *out_sig = strdup(sig);
      else
        *out_sig = NULL;
    }

    (*env)->DeleteLocalRef(env, method);

    break;

    cleanup_method:
      (*env)->DeleteLocalRef(env, method);
  }

  (*env)->DeleteLocalRef(env, methods);

  return found;
}

static inline bool amethod_init(JNIEnv *env) {
  /* INFO: Get artMethod field from Executable */
  jclass executable = (*env)->FindClass(env, "java/lang/reflect/Executable");
  if (!executable) {
    LOGW("Executable not found, falling back to FromReflectedMethod");

    if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
  } else {
    art_method_field = (*env)->GetFieldID(env, executable, "artMethod", "J");
    if (!art_method_field) {
      LOGW("Failed to find artMethod field, falling back to FromReflectedMethod");

      if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
    }

    (*env)->DeleteLocalRef(env, executable);
  }

  /* INFO: Compute ArtMethod size using Throwable constructors */
  jclass throwable = (*env)->FindClass(env, "java/lang/Throwable");
  if (!throwable) {
    LOGE("Failed to find Throwable");

    return false;
  }

  jclass class_class = (*env)->FindClass(env, "java/lang/Class");
  if (!class_class) {
    LOGE("Failed to find Class");

    (*env)->DeleteLocalRef(env, throwable);

    return false;
  }

  jmethodID get_ctors = (*env)->GetMethodID(env, class_class, "getDeclaredConstructors", "()[Ljava/lang/reflect/Constructor;");
  (*env)->DeleteLocalRef(env, class_class);

  jobjectArray constructors = (jobjectArray)(*env)->CallObjectMethod(env, throwable, get_ctors);
  (*env)->DeleteLocalRef(env, throwable);

  if (!constructors || (*env)->GetArrayLength(env, constructors) < 2) {
    LOGE("Throwable has less than 2 constructors");

    if (constructors) (*env)->DeleteLocalRef(env, constructors);

    return false;
  }

  jobject first_ctor = (*env)->GetObjectArrayElement(env, constructors, 0);
  jobject second_ctor = (*env)->GetObjectArrayElement(env, constructors, 1);

  uintptr_t first = (uintptr_t)amethod_from_reflected_method(env, first_ctor);
  uintptr_t second = (uintptr_t)amethod_from_reflected_method(env, second_ctor);

  (*env)->DeleteLocalRef(env, first_ctor);
  (*env)->DeleteLocalRef(env, second_ctor);
  (*env)->DeleteLocalRef(env, constructors);

  art_method_size = second - first;
  LOGD("ArtMethod size: %zu", art_method_size);
  if ((4 * 9 + 3 * sizeof(void *)) < art_method_size) {
    LOGE("ArtMethod size exceeds maximum assume. There may be something wrong.");

    return false;
  }

  entry_point_offset = art_method_size - sizeof(void *);
  data_offset = entry_point_offset - sizeof(void *);
  LOGD("ArtMethod entrypoint offset: %zu", entry_point_offset);
  LOGD("ArtMethod data offset: %zu", data_offset);

  /* INFO: Cache java.lang.reflect.Method method IDs */
  {
    jclass method_class = (*env)->FindClass(env, "java/lang/reflect/Method");
    if (method_class) {
      amethod_get_name_mid = (*env)->GetMethodID(env, method_class, "getName", "()Ljava/lang/String;");
      amethod_get_parameter_types_mid = (*env)->GetMethodID(env, method_class, "getParameterTypes", "()[Ljava/lang/Class;");
      amethod_get_return_type_mid = (*env)->GetMethodID(env, method_class, "getReturnType", "()Ljava/lang/Class;");

      (*env)->DeleteLocalRef(env, method_class);
    }
  }

  return true;
}

static inline void *amethod_get_data(uintptr_t self) {
  return *(void **)(self + data_offset);
}

static inline void *amethod_from_reflected_method(JNIEnv *env, jobject method) {
  if (art_method_field) {
    return (void *)(*env)->GetLongField(env, method, art_method_field);
  } else {
    return (void *)(*env)->FromReflectedMethod(env, method);
  }
}

#endif /* ART_METHOD_H */
