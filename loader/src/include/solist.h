#ifndef SOLIST_H
#define SOLIST_H

#ifdef __LP64__
  static size_t solist_size_offset = 0x18;
  static size_t solist_next_offset = 0x30;
  static size_t solist_realpath_offset = 0x1a8;
#else
  static size_t solist_size_offset = 0x90;
  static size_t solist_next_offset = 0xa4;
  static size_t solist_realpath_offset = 0x174;
#endif

typedef struct SoInfo SoInfo;

struct SoInfo {
  char data[0];
};

#define FuncType(name) void (*name)

struct pdg {
  void *(*ctor)();
  void *(*dtor)();
};

/* TODO: Explaining about what this is*/
bool solist_drop_so_path(const char *target_path);

/* TODO: Explaining about what this is */
void solist_reset_counters(size_t load, size_t unload);

#endif /* SOLIST_H */
