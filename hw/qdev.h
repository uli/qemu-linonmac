#ifndef QDEV_H
#define QDEV_H

#include "hw.h"
#include "sysemu.h"
#include "sys-queue.h"
#include "qemu-char.h"
#include "qemu-option.h"

typedef struct Property Property;

typedef struct PropertyInfo PropertyInfo;

typedef struct CompatProperty CompatProperty;

typedef struct DeviceInfo DeviceInfo;

typedef struct BusState BusState;

typedef struct BusInfo BusInfo;

/* This structure should not be accessed directly.  We declare it here
   so that it can be embedded in individual device state structures.  */
struct DeviceState {
    const char *id;
    DeviceInfo *info;
    BusState *parent_bus;
    int num_gpio_out;
    qemu_irq *gpio_out;
    int num_gpio_in;
    qemu_irq *gpio_in;
    LIST_HEAD(, BusState) child_bus;
    int num_child_bus;
    NICInfo *nd;
    LIST_ENTRY(DeviceState) sibling;
};

typedef void (*bus_dev_printfn)(Monitor *mon, DeviceState *dev, int indent);
struct BusInfo {
    const char *name;
    size_t size;
    bus_dev_printfn print_dev;
    Property *props;
};

struct BusState {
    DeviceState *parent;
    BusInfo *info;
    const char *name;
    LIST_HEAD(, DeviceState) children;
    LIST_ENTRY(BusState) sibling;
};

struct Property {
    const char   *name;
    PropertyInfo *info;
    int          offset;
    void         *defval;
};

enum PropertyType {
    PROP_TYPE_UNSPEC = 0,
    PROP_TYPE_UINT16,
    PROP_TYPE_UINT32,
    PROP_TYPE_UINT64,
    PROP_TYPE_TADDR,
    PROP_TYPE_MACADDR,
    PROP_TYPE_DRIVE,
    PROP_TYPE_CHR,
    PROP_TYPE_PTR,
};

struct PropertyInfo {
    const char *name;
    size_t size;
    enum PropertyType type;
    int (*parse)(DeviceState *dev, Property *prop, const char *str);
    int (*print)(DeviceState *dev, Property *prop, char *dest, size_t len);
};

struct CompatProperty {
    const char *driver;
    const char *property;
    const char *value;
};

/*** Board API.  This should go away once we have a machine config file.  ***/

DeviceState *qdev_create(BusState *bus, const char *name);
DeviceState *qdev_device_add(QemuOpts *opts);
void qdev_init(DeviceState *dev);
void qdev_free(DeviceState *dev);

qemu_irq qdev_get_gpio_in(DeviceState *dev, int n);
void qdev_connect_gpio_out(DeviceState *dev, int n, qemu_irq pin);

BusState *qdev_get_child_bus(DeviceState *dev, const char *name);

/*** Device API.  ***/

typedef void (*qdev_initfn)(DeviceState *dev, DeviceInfo *info);
typedef void (*SCSIAttachFn)(DeviceState *host, BlockDriverState *bdrv,
              int unit);

struct DeviceInfo {
    const char *name;
    const char *alias;
    const char *desc;
    size_t size;
    Property *props;
    int no_user;

    /* Private to qdev / bus.  */
    qdev_initfn init;
    BusInfo *bus_info;
    struct DeviceInfo *next;
};

void qdev_register(DeviceInfo *info);

/* Register device properties.  */
/* GPIO inputs also double as IRQ sinks.  */
void qdev_init_gpio_in(DeviceState *dev, qemu_irq_handler handler, int n);
void qdev_init_gpio_out(DeviceState *dev, qemu_irq *pins, int n);

void scsi_bus_new(DeviceState *host, SCSIAttachFn attach);

CharDriverState *qdev_init_chardev(DeviceState *dev);

BusState *qdev_get_parent_bus(DeviceState *dev);

/* Convery from a base type to a parent type, with compile time checking.  */
#ifdef __GNUC__
#define DO_UPCAST(type, field, dev) ( __extension__ ( { \
    char __attribute__((unused)) offset_must_be_zero[ \
        -offsetof(type, field)]; \
    container_of(dev, type, field);}))
#else
#define DO_UPCAST(type, field, dev) container_of(dev, type, field)
#endif

/*** BUS API. ***/

BusState *qbus_create(BusInfo *info, DeviceState *parent, const char *name);

#define FROM_QBUS(type, dev) DO_UPCAST(type, qbus, dev)

/*** monitor commands ***/

void do_info_qtree(Monitor *mon);
void do_info_qdm(Monitor *mon);

/*** qdev-properties.c ***/

extern PropertyInfo qdev_prop_uint16;
extern PropertyInfo qdev_prop_uint32;
extern PropertyInfo qdev_prop_uint64;
extern PropertyInfo qdev_prop_hex32;
extern PropertyInfo qdev_prop_hex64;
extern PropertyInfo qdev_prop_chr;
extern PropertyInfo qdev_prop_ptr;
extern PropertyInfo qdev_prop_macaddr;
extern PropertyInfo qdev_prop_drive;
extern PropertyInfo qdev_prop_pci_devfn;

#define typeof_field(type, field) typeof(((type *)0)->field)
#define type_check(t1,t2) ((t1*)0 - (t2*)0)

#define DEFINE_PROP(_name, _state, _field, _prop, _type) { \
        .name      = (_name),                                    \
        .info      = &(_prop),                                   \
        .offset    = offsetof(_state, _field)                    \
            + type_check(_type,typeof_field(_state, _field)),    \
        }
#define DEFINE_PROP_DEFAULT(_name, _state, _field, _defval, _prop, _type) { \
        .name      = (_name),                                           \
        .info      = &(_prop),                                          \
        .offset    = offsetof(_state, _field)                           \
            + type_check(_type,typeof_field(_state, _field)),           \
        .defval    = (_type[]) { _defval },                             \
        }

#define DEFINE_PROP_UINT16(_n, _s, _f, _d)                      \
    DEFINE_PROP_DEFAULT(_n, _s, _f, _d, qdev_prop_uint16, uint16_t)
#define DEFINE_PROP_UINT32(_n, _s, _f, _d)                      \
    DEFINE_PROP_DEFAULT(_n, _s, _f, _d, qdev_prop_uint32, uint32_t)
#define DEFINE_PROP_UINT64(_n, _s, _f, _d)                      \
    DEFINE_PROP_DEFAULT(_n, _s, _f, _d, qdev_prop_uint64, uint64_t)
#define DEFINE_PROP_HEX32(_n, _s, _f, _d)                       \
    DEFINE_PROP_DEFAULT(_n, _s, _f, _d, qdev_prop_hex32, uint32_t)
#define DEFINE_PROP_HEX64(_n, _s, _f, _d)                       \
    DEFINE_PROP_DEFAULT(_n, _s, _f, _d, qdev_prop_hex64, uint64_t)
#define DEFINE_PROP_PCI_DEVFN(_n, _s, _f, _d)                   \
    DEFINE_PROP_DEFAULT(_n, _s, _f, _d, qdev_prop_pci_devfn, uint32_t)

#define DEFINE_PROP_PTR(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_ptr, void*)
#define DEFINE_PROP_CHR(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_chr, CharDriverState*)
#define DEFINE_PROP_DRIVE(_n, _s, _f)             \
    DEFINE_PROP(_n, _s, _f, qdev_prop_drive, DriveInfo*)
#define DEFINE_PROP_MACADDR(_n, _s, _f)         \
    DEFINE_PROP(_n, _s, _f, qdev_prop_macaddr, uint8_t[6])

#define DEFINE_PROP_END_OF_LIST()               \
    {}

/* Set properties between creation and init.  */
void *qdev_get_prop_ptr(DeviceState *dev, Property *prop);
int qdev_prop_parse(DeviceState *dev, const char *name, const char *value);
void qdev_prop_set(DeviceState *dev, const char *name, void *src, enum PropertyType type);
void qdev_prop_set_uint16(DeviceState *dev, const char *name, uint16_t value);
void qdev_prop_set_uint32(DeviceState *dev, const char *name, uint32_t value);
void qdev_prop_set_uint64(DeviceState *dev, const char *name, uint64_t value);
void qdev_prop_set_chr(DeviceState *dev, const char *name, CharDriverState *value);
void qdev_prop_set_drive(DeviceState *dev, const char *name, DriveInfo *value);
/* FIXME: Remove opaque pointer properties.  */
void qdev_prop_set_ptr(DeviceState *dev, const char *name, void *value);
void qdev_prop_set_defaults(DeviceState *dev, Property *props);

void qdev_prop_register_compat(CompatProperty *props);
void qdev_prop_set_compat(DeviceState *dev);

/* This is a nasty hack to allow passing a NULL bus to qdev_create.  */
extern struct BusInfo system_bus_info;

#endif
