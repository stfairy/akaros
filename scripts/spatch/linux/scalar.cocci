@@
typedef u8;
typedef uint8_t;
@@
-u8
+uint8_t

@@
typedef u16;
typedef uint16_t;
@@
-u16
+uint16_t

@@
typedef __u16;
@@
-__u16
+uint16_t

@@
typedef u32;
typedef uint32_t;
@@
-u32
+uint32_t

@@
typedef u64;
typedef uint64_t;
@@
-u64
+uint64_t

@@
typedef cycle_t;
@@
-cycle_t
+uint64_t

@@
typedef s8;
typedef int8_t;
@@
-s8
+int8_t

@@
typedef s16;
typedef int16_t;
@@
-s16
+int16_t

@@
typedef s32;
typedef int32_t;
@@
-s32
+int32_t

@@
typedef s64;
typedef int64_t;
@@
-s64
+int64_t

@@
typedef uint;
@@
-uint
+unsigned int

@@
typedef __sum16;
@@
-__sum16
+uint16_t

@@
typedef __wsum;
@@
-__wsum
+uint32_t

@@
@@
-ETH_ALEN
+Eaddrlen

@@
@@
-ETH_HLEN
+ETHERHDRSIZE

@@
@@
 struct
-pci_dev
+pci_device

@@
@@
 struct
-net_device
+ether

@@
struct pci_device *p;
@@
-p->irq
+p->irqline

@@
struct ether *p;
@@
-p->features
+p->feat

@@
struct ether *p;
expression E;
@@
-p->vlan_features = E;

@@
struct ether *p;
expression E;
@@
-p->hw_enc_features = E;

// yes, that's a max max trans unit...
@@
struct ether *p;
@@
-p->mtu
+p->maxmtu

@@
struct ether *p;
@@
-p->dev_addr
+p->ea

@@
struct ether *p;
expression E;
@@
-p->dev_port = E;

@@
struct ether *p;
expression E;
@@
-p->addr_len = E;

@@
struct ether *p;
expression E;
@@
-p->netdev_ops = E;

@@
struct ether *p;
expression E;
@@
-p->watchdog_timeo = E;

@@
struct ether *p;
expression E;
@@
-p->ethtool_ops = E;
