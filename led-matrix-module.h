#include <linux/module.h>

// Indicates which rows are completly lit
ssize_t rows_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

ssize_t rows_store(struct kobject *kobj, struct kobj_attribute *attr,
                   const char *buf, size_t count);

// Indicates which columns are completly lit
ssize_t col_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

ssize_t col_store(struct kobject *kobj, struct kobj_attribute *attr,
                  const char *buf, size_t count);

ssize_t character_show(struct kobject *kobj, struct kobj_attribute *attr,
                       char *buf);

ssize_t character_store(struct kobject *kobj, struct kobj_attribute *attr,
                        const char *buf, size_t count);

ssize_t fps_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

ssize_t fps_store(struct kobject *kobj, struct kobj_attribute *attr,
                  const char *buf, size_t count);

ssize_t pixels_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf);

ssize_t pixels_store(struct kobject *kobj, struct kobj_attribute *attr,
                     const char *buf, size_t count);

ssize_t string_show(struct kobject *kobj, struct kobj_attribute *attr,
                    char *buf);

ssize_t string_store(struct kobject *kobj, struct kobj_attribute *attr,
                     const char *buf, size_t count);