/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 * Copyright (c) 2007 Philip Craig <philipc@snapgear.com>
 * Copyright (c) 2007 Secure Computing Corporation
 * Copyright (c) 2008 Patrick McHardy <kaber@trash.net>
 */

#ifndef NETLINK_LOG_MSG_H_
#define NETLINK_LOG_MSG_H_

#include <netlink/netlink.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nlmsghdr;
struct nfnl_log_msg;
struct nfnl_ct;

extern struct nl_object_ops log_msg_obj_ops;

/* General */
extern struct nfnl_log_msg *nfnl_log_msg_alloc(void);
extern int		nfnlmsg_log_msg_parse(struct nlmsghdr *,
					      struct nfnl_log_msg **);

extern void		nfnl_log_msg_get(struct nfnl_log_msg *);
extern void		nfnl_log_msg_put(struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_family(struct nfnl_log_msg *, uint8_t);
extern uint8_t		nfnl_log_msg_get_family(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_hwproto(struct nfnl_log_msg *, uint16_t);
extern int		nfnl_log_msg_test_hwproto(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_hwproto(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_hook(struct nfnl_log_msg *, uint8_t);
extern int		nfnl_log_msg_test_hook(const struct nfnl_log_msg *);
extern uint8_t		nfnl_log_msg_get_hook(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_mark(struct nfnl_log_msg *, uint32_t);
extern int		nfnl_log_msg_test_mark(const struct nfnl_log_msg *);
extern uint32_t		nfnl_log_msg_get_mark(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_timestamp(struct nfnl_log_msg *,
					       struct timeval *);
extern const struct timeval *nfnl_log_msg_get_timestamp(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_indev(struct nfnl_log_msg *, uint32_t);
extern uint32_t		nfnl_log_msg_get_indev(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_outdev(struct nfnl_log_msg *, uint32_t);
extern uint32_t		nfnl_log_msg_get_outdev(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_physindev(struct nfnl_log_msg *, uint32_t);
extern uint32_t		nfnl_log_msg_get_physindev(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_physoutdev(struct nfnl_log_msg *, uint32_t);
extern uint32_t		nfnl_log_msg_get_physoutdev(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_hwaddr(struct nfnl_log_msg *, uint8_t *, int);
extern const uint8_t *	nfnl_log_msg_get_hwaddr(const struct nfnl_log_msg *, int *);

extern int		nfnl_log_msg_set_payload(struct nfnl_log_msg *, uint8_t *, int);
extern const void *	nfnl_log_msg_get_payload(const struct nfnl_log_msg *, int *);

extern int		nfnl_log_msg_set_prefix(struct nfnl_log_msg *, void *);
extern const char *	nfnl_log_msg_get_prefix(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_uid(struct nfnl_log_msg *, uint32_t);
extern int		nfnl_log_msg_test_uid(const struct nfnl_log_msg *);
extern uint32_t		nfnl_log_msg_get_uid(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_gid(struct nfnl_log_msg *, uint32_t);
extern int		nfnl_log_msg_test_gid(const struct nfnl_log_msg *);
extern uint32_t		nfnl_log_msg_get_gid(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_seq(struct nfnl_log_msg *, uint32_t);
extern int		nfnl_log_msg_test_seq(const struct nfnl_log_msg *);
extern uint32_t		nfnl_log_msg_get_seq(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_seq_global(struct nfnl_log_msg *, uint32_t);
extern int		nfnl_log_msg_test_seq_global(const struct nfnl_log_msg *);
extern uint32_t		nfnl_log_msg_get_seq_global(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_hwtype(struct nfnl_log_msg *, uint16_t);
extern int		nfnl_log_msg_test_hwtype(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_hwtype(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_hwlen(struct nfnl_log_msg *, uint16_t);
extern int		nfnl_log_msg_test_hwlen(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_hwlen(const struct nfnl_log_msg *);

extern int		nfnl_log_msg_set_hwheader(struct nfnl_log_msg *, void *, int);
extern int		nfnl_log_msg_test_hwheader(const struct nfnl_log_msg *);
extern const void *	nfnl_log_msg_get_hwheader(const struct nfnl_log_msg *, int *);

extern void		nfnl_log_msg_set_vlan_proto(struct nfnl_log_msg *, uint16_t);
extern int		nfnl_log_msg_test_vlan_proto(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_vlan_proto(const struct nfnl_log_msg *);
extern void		nfnl_log_msg_set_vlan_tag(struct nfnl_log_msg *, uint16_t);
extern int		nfnl_log_msg_test_vlan_tag(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_vlan_tag(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_vlan_id(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_vlan_cfi(const struct nfnl_log_msg *);
extern uint16_t		nfnl_log_msg_get_vlan_prio(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_ct_info(struct nfnl_log_msg *, uint32_t);
extern int		nfnl_log_msg_test_ct_info(const struct nfnl_log_msg *);
extern uint32_t		nfnl_log_msg_get_ct_info(const struct nfnl_log_msg *);

extern void		nfnl_log_msg_set_ct(struct nfnl_log_msg *, struct nfnl_ct *);
extern int		nfnl_log_msg_test_ct(const struct nfnl_log_msg *);
extern struct nfnl_ct * nfnl_log_msg_get_ct(const struct nfnl_log_msg *);

#ifdef __cplusplus
}
#endif

#endif
