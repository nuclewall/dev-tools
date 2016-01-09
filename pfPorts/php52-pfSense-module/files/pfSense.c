/*
        Copyright (C) 2010 Ermal Lu�i
        All rights reserved.

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice,
           this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright
           notice, this list of conditions and the following disclaimer in the
           documentation and/or other materials provided with the distribution.

        THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
        INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
        AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
        AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
        OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
        SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
        INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
        CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
        ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
        POSSIBILITY OF SUCH DAMAGE.

*/

/*

Functions copied from util.c and modem.c of mpd5 source are protected by
this copyright.
They are ExclusiveOpenDevice/ExclusiveCloseDevice and
OpenSerialDevice.

Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.

Subject to the following obligations and disclaimer of warranty,
use and redistribution of this software, in source or object code
forms, with or without modifications are expressly permitted by
Whistle Communications; provided, however, that:   (i) any and
all reproductions of the source or object code must include the
copyright notice above and the following disclaimer of warranties;
and (ii) no rights are granted, in any manner or form, to use
Whistle Communications, Inc. trademarks, including the mark "WHISTLE
COMMUNICATIONS" on advertising, endorsements, or otherwise except
as such appears in the above copyright notice or in the software.

THIS SOFTWARE IS BEING PROVIDED BY WHISTLE COMMUNICATIONS "AS IS",
AND TO THE MAXIMUM EXTENT PERMITTED BY LAW, WHISTLE COMMUNICATIONS
MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED,
REGARDING THIS SOFTWARE, INCLUDING WITHOUT LIMITATION, ANY AND
ALL IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE, OR NON-INFRINGEMENT.  WHISTLE COMMUNICATIONS DOES NOT
WARRANT, GUARANTEE, OR MAKE ANY REPRESENTATIONS REGARDING THE USE
OF, OR THE RESULTS OF THE USE OF THIS SOFTWARE IN TERMS OF ITS
CORRECTNESS, ACCURACY, RELIABILITY OR OTHERWISE.  IN NO EVENT
SHALL WHISTLE COMMUNICATIONS BE LIABLE FOR ANY DAMAGES RESULTING
FROM OR ARISING OUT OF ANY USE OF THIS SOFTWARE, INCLUDING WITHOUT
LIMITATION, ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
PUNITIVE, OR CONSEQUENTIAL DAMAGES, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES, LOSS OF USE, DATA OR PROFITS, HOWEVER CAUSED
AND UNDER ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF WHISTLE COMMUNICATIONS
IS ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#define IS_EXT_MODULE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_pfSense.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/ethernet.h>
#include <net/if_types.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_vlan_var.h>
#include <net/if_dl.h>
#include <net/if_mib.h>
#include <net/if_bridgevar.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <net/pfvar.h>

#include <netinet/ip_fw.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/ethernet.h>

#include <netgraph/ng_message.h>
#include <netgraph/ng_ether.h>

#include <vm/vm_param.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

#include <glob.h>
#include <termios.h>
#include <poll.h>

#include <netgraph.h>

int pfSense_dhcpd;

ZEND_DECLARE_MODULE_GLOBALS(pfSense)

static function_entry pfSense_functions[] = {
    PHP_FE(pfSense_get_interface_info, NULL)
    PHP_FE(pfSense_get_interface_addresses, NULL)
    PHP_FE(pfSense_getall_interface_addresses, NULL)
    PHP_FE(pfSense_get_interface_stats, NULL)
    PHP_FE(pfSense_get_pf_stats, NULL)
    PHP_FE(pfSense_get_os_hw_data, NULL)
    PHP_FE(pfSense_get_os_kern_data, NULL)
    PHP_FE(pfSense_vlan_create, NULL)
    PHP_FE(pfSense_interface_rename, NULL)
    PHP_FE(pfSense_interface_mtu, NULL)
    PHP_FE(pfSense_bridge_add_member, NULL)
    PHP_FE(pfSense_bridge_del_member, NULL)
    PHP_FE(pfSense_bridge_member_flags, NULL)
    PHP_FE(pfSense_interface_listget, NULL)
    PHP_FE(pfSense_interface_create, NULL)
    PHP_FE(pfSense_interface_destroy, NULL)
    PHP_FE(pfSense_interface_flags, NULL)
    PHP_FE(pfSense_interface_capabilities, NULL)
    PHP_FE(pfSense_interface_setaddress, NULL)
    PHP_FE(pfSense_interface_deladdress, NULL)
    PHP_FE(pfSense_ngctl_name, NULL)
    PHP_FE(pfSense_ngctl_attach, NULL)
    PHP_FE(pfSense_ngctl_detach, NULL)
    PHP_FE(pfSense_get_modem_devices, NULL)
    PHP_FE(pfSense_sync, NULL)
#ifdef DHCP_INTEGRATION
    PHP_FE(pfSense_open_dhcpd, NULL)
    PHP_FE(pfSense_close_dhcpd, NULL)
    PHP_FE(pfSense_register_lease, NULL)
    PHP_FE(pfSense_delete_lease, NULL)
#endif
#ifdef IPFW_FUNCTIONS
   PHP_FE(pfSense_ipfw_getTablestats, NULL)
#endif
    {NULL, NULL, NULL}
};

zend_module_entry pfSense_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_PFSENSE_WORLD_EXTNAME,
    pfSense_functions,
    PHP_MINIT(pfSense_socket),
    PHP_MSHUTDOWN(pfSense_socket_close),
    NULL,
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_PFSENSE_WORLD_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_PFSENSE
ZEND_GET_MODULE(pfSense)
#endif

#ifdef DHCP_INTEGRATION
static void
php_pfSense_destroy_dhcpd(zend_rsrc_list_entry *rsrc TSRMLS_DC)
{
	omapi_data *conn = (omapi_data *)rsrc->ptr;

	if (conn)
		efree(conn);
}
#endif

/* interface management code */
static int
pfi_get_ifaces(int dev, const char *filter, struct pfi_kif *buf, int *size)
{
	struct pfioc_iface io;
	caddr_t buff[sizeof(struct pfi_kif) + 1] = { 0 };

	bzero(&io, sizeof io);
	if (filter != NULL)
		if (strlcpy(io.pfiio_name, filter, sizeof(io.pfiio_name)) >=
		    sizeof(io.pfiio_name)) {
			errno = EINVAL;
			return (-1);
		}
	io.pfiio_buffer = buf;
	io.pfiio_esize = sizeof(struct pfi_kif);
	io.pfiio_size = *size;
	if (ioctl(dev, DIOCIGETIFACES, &io))
		return (-1);
	*size = io.pfiio_size;
	return (0);
}

PHP_MINIT_FUNCTION(pfSense_socket)
{
	int csock;

	PFSENSE_G(s) = socket(AF_LOCAL, SOCK_DGRAM, 0);
	if (PFSENSE_G(s) < 0)
		return FAILURE;

	PFSENSE_G(inets) = socket(AF_INET, SOCK_DGRAM, 0);
	if (PFSENSE_G(inets) < 0) {
		close(PFSENSE_G(s));
		return FAILURE;
	}

	if (geteuid() == 0 || getuid() == 0) {
#ifdef IPFW_FUNCTIONS
		PFSENSE_G(ipfw) = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
		if (PFSENSE_G(ipfw) < 0) {
			close(PFSENSE_G(s));
			close(PFSENSE_G(inets));
			return FAILURE;
		} else
			fcntl(PFSENSE_G(ipfw), F_SETFD, fcntl(PFSENSE_G(ipfw), F_GETFD, 0) | FD_CLOEXEC);
	
#endif
		/* Create a new socket node */
		if (NgMkSockNode(NULL, &csock, NULL) < 0)
			csock = -1;
		else
			fcntl(csock, F_SETFD, fcntl(csock, F_GETFD, 0) | FD_CLOEXEC);

		PFSENSE_G(csock) = csock;

#ifdef DHCP_INTEGRATION
		pfSense_dhcpd = zend_register_list_destructors_ex(php_pfSense_destroy_dhcpd, NULL, PHP_PFSENSE_RES_NAME, module_number);
		dhcpctl_initialize();
		omapi_init();
#endif
	} else
		PFSENSE_G(csock) = -1;

	/* Don't leak these sockets to child processes */
	fcntl(PFSENSE_G(s), F_SETFD, fcntl(PFSENSE_G(s), F_GETFD, 0) | FD_CLOEXEC);
	fcntl(PFSENSE_G(inets), F_SETFD, fcntl(PFSENSE_G(inets), F_GETFD, 0) | FD_CLOEXEC);

	REGISTER_LONG_CONSTANT("IFF_UP", IFF_UP, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFF_LINK0", IFF_LINK0, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFF_LINK1", IFF_LINK1, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFF_LINK2", IFF_LINK2, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFF_NOARP", IFF_NOARP, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFF_STATICARP", IFF_STATICARP, CONST_PERSISTENT | CONST_CS);
#if defined(IFF_IPFW_FILTER)
	REGISTER_LONG_CONSTANT("IFF_IPFW_FILTER", IFF_IPFW_FILTER, CONST_PERSISTENT | CONST_CS);
#endif
	REGISTER_LONG_CONSTANT("IFCAP_RXCSUM", IFCAP_RXCSUM, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_TXCSUM", IFCAP_TXCSUM, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_POLLING", IFCAP_POLLING, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_TSO", IFCAP_TSO, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_LRO", IFCAP_LRO, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_WOL", IFCAP_WOL, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_WOL_UCAST", IFCAP_WOL_UCAST, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_WOL_MCAST", IFCAP_WOL_MCAST, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_WOL_MAGIC", IFCAP_WOL_MAGIC, CONST_PERSISTENT | CONST_CS);

	REGISTER_LONG_CONSTANT("IFCAP_VLAN_HWTAGGING", IFCAP_VLAN_HWTAGGING, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_VLAN_MTU", IFCAP_VLAN_MTU, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFCAP_VLAN_HWFILTER", IFCAP_VLAN_HWFILTER, CONST_PERSISTENT | CONST_CS);
#ifdef IFCAP_VLAN_HWCSUM
	REGISTER_LONG_CONSTANT("IFCAP_VLAN_HWCSUM", IFCAP_VLAN_HWCSUM, CONST_PERSISTENT | CONST_CS);
#endif
#ifdef IFCAP_VLAN_HWTSO
	REGISTER_LONG_CONSTANT("IFCAP_VLAN_HWTSO", IFCAP_VLAN_HWTSO, CONST_PERSISTENT | CONST_CS);
#endif

	REGISTER_LONG_CONSTANT("IFBIF_LEARNING", IFBIF_LEARNING, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_DISCOVER", IFBIF_DISCOVER, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_STP", IFBIF_STP, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_SPAN", IFBIF_SPAN, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_STICKY", IFBIF_STICKY, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_BSTP_EDGE", IFBIF_BSTP_EDGE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_BSTP_AUTOEDGE", IFBIF_BSTP_AUTOEDGE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_BSTP_PTP", IFBIF_BSTP_PTP, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_BSTP_AUTOPTP", IFBIF_BSTP_AUTOPTP, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_BSTP_ADMEDGE", IFBIF_BSTP_ADMEDGE, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_BSTP_ADMCOST", IFBIF_BSTP_ADMCOST, CONST_PERSISTENT | CONST_CS);
	REGISTER_LONG_CONSTANT("IFBIF_PRIVATE", IFBIF_PRIVATE, CONST_PERSISTENT | CONST_CS);

	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(pfSense_socket_close)
{
	if (PFSENSE_G(csock) != -1)
		close(PFSENSE_G(csock));
	if (PFSENSE_G(inets) != -1)
		close(PFSENSE_G(inets));
	if (PFSENSE_G(s) != -1)
		close(PFSENSE_G(s));

	return SUCCESS;
}

#ifdef IPFW_FUNCTIONS
/* Stolen from ipfw2.c code */
static unsigned long long
pfSense_align_uint64(const uint64_t *pll)
{
        uint64_t ret;

        bcopy (pll, &ret, sizeof(ret));
        return ret;
}

PHP_FUNCTION(pfSense_ipfw_getTablestats)
{
	ipfw_table_entry ent;
	socklen_t size;
	int mask = 0, table;
	char *ip, *name;
	int ip_len, name_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sls|l", &name, &name_len, &table, &ip, &ip_len, &mask) == FAILURE) {
		RETURN_FALSE;
	}

	/* XXX: Use context data! */

	if (strchr(ip, ':')) {
		if (!inet_pton(AF_INET6, ip, &ent.addr))
			RETURN_FALSE;
	} else if (!inet_pton(AF_INET, ip, &ent.addr)) {
		RETURN_FALSE;
	}

	if (mask)
		ent.masklen = mask;
	else
		ent.masklen = 32;
	size = sizeof(ent);
	ent.tbl = table;
	if (getsockopt(PFSENSE_G(ipfw), IPPROTO_IP, IP_FW_TABLE_GET_ENTRY, &ent, &size) < 0)
		RETURN_FALSE;

	array_init(return_value);
	add_assoc_long(return_value, "packets", pfSense_align_uint64(&ent.packets));
	add_assoc_long(return_value, "bytes", pfSense_align_uint64(&ent.bytes));
	add_assoc_long(return_value, "timestamp", ent.timestamp);
	add_assoc_long(return_value, "dnpipe", ent.value);
}
#endif

#ifdef DHCP_INTEGRATION
PHP_FUNCTION(pfSense_open_dhcpd)
{
	omapi_data *data;
	char *key, *addr, *name;
	int key_len, addr_len, name_len, port;
	dhcpctl_status status;
	dhcpctl_handle auth = dhcpctl_null_handle, conn = dhcpctl_null_handle;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sssl", &name, &name_len, &key, &key_len, &addr, &addr_len, &port) == FAILURE) {
		RETURN_FALSE;
	}

	status = dhcpctl_new_authenticator(&auth, name, "hmac-md5", key, key_len);
	if (status != ISC_R_SUCCESS) {
		//php_printf("Failed to get aythenticator: %s - %s\n", isc_result_totext(status), key);
		RETURN_NULL();
	}

	status = dhcpctl_connect(&conn, addr, port, auth);
	if (status != ISC_R_SUCCESS) {
		//php_printf("Error occured during connecting: %s\n", isc_result_totext(status));
		RETURN_NULL();
	}

	data = emalloc(sizeof(*data));
	data->handle = conn;

	ZEND_REGISTER_RESOURCE(return_value, data, pfSense_dhcpd);
}

PHP_FUNCTION(pfSense_register_lease)
{
        dhcpctl_status status = ISC_R_SUCCESS;
        dhcpctl_status status2 = ISC_R_SUCCESS;
        dhcpctl_handle hp = NULL;
        struct ether_addr *ds;
        struct in_addr nds;
	char *mac, *ip, *name;
	int mac_len, ip_len, name_len;
	zval *res;
	omapi_data *conn;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zsss", &res, &name, &name_len, &mac, &mac_len, &ip, &ip_len) == FAILURE) {
		RETURN_FALSE;
	}

	ZEND_FETCH_RESOURCE(conn, omapi_data *, &res, -1, PHP_PFSENSE_RES_NAME, pfSense_dhcpd);
	ZEND_VERIFY_RESOURCE(conn);

        if ((status = dhcpctl_new_object(&hp, conn->handle, "host")) != ISC_R_SUCCESS) {
		//php_printf("1Error occured during connecting: %s\n", isc_result_totext(status));
		RETURN_FALSE;
	}

	inet_aton(ip, &nds);
        if ((status = dhcpctl_set_data_value(hp, (char *)&nds, sizeof(struct in_addr), "ip-address")) != ISC_R_SUCCESS) {
		//php_printf("3Error occured during connecting: %s\n", isc_result_totext(status));
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}

        if ((status = dhcpctl_set_string_value(hp, name, "name")) != ISC_R_SUCCESS) {
		//php_printf("4Error occured during connecting: %s\n", isc_result_totext(status));
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}

        if (!(ds = ether_aton(mac)))
		RETURN_FALSE;
        if ((status = dhcpctl_set_data_value(hp, (u_char *)ds, sizeof(struct ether_addr), "hardware-address")) != ISC_R_SUCCESS) {
		//php_printf("2Error occured during connecting: %s\n", isc_result_totext(status));
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}

	if ((status= dhcpctl_set_int_value(hp, 1,"hardware-type")) != ISC_R_SUCCESS)  {
		//php_printf("2Error occured during connecting: %s\n", isc_result_totext(status));
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}

	//php_printf("Coonection handle %d\n", conn->handle);
        if ((status = dhcpctl_open_object(hp, conn->handle, DHCPCTL_CREATE|DHCPCTL_EXCL)) != ISC_R_SUCCESS) {
		//php_printf("5Error occured during connecting: %s\n", isc_result_totext(status));
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if ((status = dhcpctl_wait_for_completion(hp, &status2)) != ISC_R_SUCCESS) {
		//php_printf("6Error occured during connecting: %s-  %s\n", isc_result_totext(status), isc_result_totext(status2));
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if (status2 != ISC_R_SUCCESS) {
		//php_printf("7Error occured during connecting: %s\n", isc_result_totext(status2));
                omapi_object_dereference(&hp,__FILE__,__LINE__);
                RETURN_FALSE;
        }

        omapi_object_dereference(&hp,__FILE__,__LINE__);

        RETURN_TRUE;
}

PHP_FUNCTION(pfSense_delete_lease)
{
	dhcpctl_status status;
        dhcpctl_status status2;
        dhcpctl_handle hp = NULL;
        dhcpctl_data_string ds = NULL;
        omapi_data_string_t *nds;
        char *mac;
	int mac_len;
	zval *res;
	omapi_data *conn;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &mac, &mac_len) == FAILURE) {
                RETURN_FALSE;
        }

	ZEND_FETCH_RESOURCE(conn, omapi_data *, &res, -1, PHP_PFSENSE_RES_NAME, pfSense_dhcpd);
        ZEND_VERIFY_RESOURCE(conn);

        if ((status = dhcpctl_new_object(&hp, conn->handle, "host")))
		RETURN_FALSE;

        if (mac) {
                omapi_data_string_new(&ds, sizeof(struct ether_addr), __FILE__, __LINE__);
                memcpy(ds->value,ether_aton(mac),sizeof(struct ether_addr));
                if ((status = dhcpctl_set_value(hp, ds, "hardware-address"))) {
			omapi_object_dereference(&hp,__FILE__,__LINE__);
			RETURN_FALSE;
		}
        } else
		RETURN_FALSE;
        
        if ((status = dhcpctl_open_object(hp, conn->handle, 0))) {
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if ((status = dhcpctl_wait_for_completion(hp, &status2))) {
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if (status2) {
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if ((status = dhcpctl_object_remove(conn->handle, hp))) {
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if ((status = dhcpctl_wait_for_completion(hp, &status2))) {
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        if (status2) {
		omapi_object_dereference(&hp,__FILE__,__LINE__);
		RETURN_FALSE;
	}
        omapi_object_dereference(&hp,__FILE__,__LINE__);

	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_close_dhcpd)
{
	zval *data;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &data) == FAILURE) {
		RETURN_FALSE;
	}

	zend_list_delete(Z_LVAL_P(data));

	RETURN_TRUE;
}
#endif

PHP_FUNCTION(pfSense_getall_interface_addresses)
{
	struct ifaddrs *ifdata, *mb;
        struct if_data *md;
	struct sockaddr_in *tmp;
        char outputbuf[132];
        char *ifname;
        int ifname_len, s;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ifname, &ifname_len) == FAILURE)
                RETURN_NULL();

	getifaddrs(&ifdata);
        if (ifdata == NULL)
                RETURN_NULL();

	array_init(return_value);

        for(mb = ifdata; mb != NULL; mb = mb->ifa_next) {
		if (mb == NULL)
                        continue;
		if (ifname_len != strlen(mb->ifa_name))
                        continue;
                if (strncmp(ifname, mb->ifa_name, ifname_len) != 0)
                        continue;
		if (mb->ifa_addr == NULL)
			continue;

		switch (mb->ifa_addr->sa_family) {
		case AF_INET:
			bzero(outputbuf, sizeof outputbuf);
			tmp = (struct sockaddr_in *)mb->ifa_addr;
			inet_ntop(AF_INET, (void *)&tmp->sin_addr, outputbuf, 132);
			tmp = (struct sockaddr_in *)mb->ifa_netmask;
			unsigned char mask;
			const unsigned char *byte = (unsigned char *)&tmp->sin_addr.s_addr;
			int i = 0, n = sizeof(tmp->sin_addr.s_addr);
			while (n--) {
				mask = ((unsigned char)-1 >> 1) + 1;
					do {
						if (mask & byte[n])
							i++;
						mask >>= 1;
					} while (mask);
			}
			snprintf(outputbuf + strlen(outputbuf), sizeof(outputbuf) - strlen(outputbuf), "/%d", i);
			add_next_index_string(return_value, outputbuf, 1);
			break;
		}
	}
}

PHP_FUNCTION(pfSense_get_interface_addresses)
{
	struct ifaddrs *ifdata, *mb;
        struct if_data *md;
	struct sockaddr_in *tmp;
        struct sockaddr_dl *tmpdl;
	struct ifreq ifr;
        char outputbuf[128];
        char *ifname;
        int ifname_len, s, addresscnt = 0;
	zval *caps;
	zval *encaps;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ifname, &ifname_len) == FAILURE)
                RETURN_NULL();

	getifaddrs(&ifdata);
        if (ifdata == NULL)
                RETURN_NULL();

	array_init(return_value);

        for(mb = ifdata; mb != NULL; mb = mb->ifa_next) {
		if (mb == NULL)
                        continue;
		if (ifname_len != strlen(mb->ifa_name))
                        continue;
                if (strncmp(ifname, mb->ifa_name, ifname_len) != 0)
                        continue;

	if (mb->ifa_flags & IFF_UP)
		add_assoc_string(return_value, "status", "up", 1);
	else
		add_assoc_string(return_value, "status", "down", 1);
	if (mb->ifa_flags & IFF_LINK0)
		add_assoc_long(return_value, "link0", 1);
	if (mb->ifa_flags & IFF_LINK1)
		add_assoc_long(return_value, "link1", 1);
	if (mb->ifa_flags & IFF_LINK2)
		add_assoc_long(return_value, "link2", 1);
	if (mb->ifa_flags & IFF_MULTICAST)
		add_assoc_long(return_value, "multicast", 1);
	if (mb->ifa_flags & IFF_LOOPBACK)
                add_assoc_long(return_value, "loopback", 1);
	if (mb->ifa_flags & IFF_POINTOPOINT)
                add_assoc_long(return_value, "pointtopoint", 1);
	if (mb->ifa_flags & IFF_PROMISC)
                add_assoc_long(return_value, "promisc", 1);
	if (mb->ifa_flags & IFF_PPROMISC)
                add_assoc_long(return_value, "permanentpromisc", 1);
	if (mb->ifa_flags & IFF_OACTIVE)
                add_assoc_long(return_value, "oactive", 1);
	if (mb->ifa_flags & IFF_ALLMULTI)
                add_assoc_long(return_value, "allmulti", 1);
	if (mb->ifa_flags & IFF_SIMPLEX)
                add_assoc_long(return_value, "simplex", 1);
	if (mb->ifa_data != NULL) {
		md = mb->ifa_data;
		if (md->ifi_link_state == LINK_STATE_UP)
                	add_assoc_long(return_value, "linkstateup", 1);
		//add_assoc_long(return_value, "hwassistflag", md->ifi_hwassist);
		add_assoc_long(return_value, "mtu", md->ifi_mtu);
		switch (md->ifi_type) {
		case IFT_IEEE80211:
			add_assoc_string(return_value, "iftype", "wireless", 1);
			break;
		case IFT_ETHER:
		case IFT_FASTETHER:
		case IFT_FASTETHERFX:
		case IFT_GIGABITETHERNET:
			add_assoc_string(return_value, "iftype", "ether", 1);
			break;
		case IFT_L2VLAN:
			add_assoc_string(return_value, "iftype", "vlan", 1);
			break;
		case IFT_BRIDGE:
			add_assoc_string(return_value, "iftype", "bridge", 1);
			break;
		case IFT_TUNNEL:
		case IFT_GIF:
		case IFT_FAITH:
		case IFT_ENC:
		case IFT_PFLOG: 
		case IFT_PFSYNC:
			add_assoc_string(return_value, "iftype", "virtual", 1);
			break;
		case IFT_CARP:
			add_assoc_string(return_value, "iftype", "carp", 1);
			break;
		default:
			add_assoc_string(return_value, "iftype", "other", 1);
		}
	}
	ALLOC_INIT_ZVAL(caps);
	ALLOC_INIT_ZVAL(encaps);
	array_init(caps);
	array_init(encaps);
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(PFSENSE_G(s), SIOCGIFCAP, (caddr_t)&ifr) == 0) {
		add_assoc_long(caps, "flags", ifr.ifr_reqcap);
		if (ifr.ifr_reqcap & IFCAP_POLLING)
			add_assoc_long(caps, "polling", 1);
		if (ifr.ifr_reqcap & IFCAP_RXCSUM)
			add_assoc_long(caps, "rxcsum", 1);
		if (ifr.ifr_reqcap & IFCAP_TXCSUM)
			add_assoc_long(caps, "txcsum", 1);
		if (ifr.ifr_reqcap & IFCAP_VLAN_MTU)
			add_assoc_long(caps, "vlanmtu", 1);
		if (ifr.ifr_reqcap & IFCAP_JUMBO_MTU)
			add_assoc_long(caps, "jumbomtu", 1);
		if (ifr.ifr_reqcap & IFCAP_VLAN_HWTAGGING)
			add_assoc_long(caps, "vlanhwtag", 1);
		if (ifr.ifr_reqcap & IFCAP_VLAN_HWCSUM)
               		add_assoc_long(caps, "vlanhwcsum", 1);
		if (ifr.ifr_reqcap & IFCAP_TSO4)
               		add_assoc_long(caps, "tso4", 1);
		if (ifr.ifr_reqcap & IFCAP_TSO6)
               		add_assoc_long(caps, "tso6", 1);
		if (ifr.ifr_reqcap & IFCAP_LRO)
               		add_assoc_long(caps, "lro", 1);
		if (ifr.ifr_reqcap & IFCAP_WOL_UCAST)
               		add_assoc_long(caps, "wolucast", 1);
		if (ifr.ifr_reqcap & IFCAP_WOL_MCAST)
               		add_assoc_long(caps, "wolmcast", 1);
		if (ifr.ifr_reqcap & IFCAP_WOL_MAGIC)
               		add_assoc_long(caps, "wolmagic", 1);
		if (ifr.ifr_reqcap & IFCAP_TOE4)
              		add_assoc_long(caps, "toe4", 1);
		if (ifr.ifr_reqcap & IFCAP_TOE6)
               		add_assoc_long(caps, "toe6", 1);
		if (ifr.ifr_reqcap & IFCAP_VLAN_HWFILTER)
               		add_assoc_long(caps, "vlanhwfilter", 1);
#if 0
		if (ifr.ifr_reqcap & IFCAP_POLLING_NOCOUNT)
               		add_assoc_long(caps, "pollingnocount", 1);
#endif
		add_assoc_long(encaps, "flags", ifr.ifr_curcap);
		if (ifr.ifr_curcap & IFCAP_POLLING)
                	add_assoc_long(encaps, "polling", 1);
                if (ifr.ifr_curcap & IFCAP_RXCSUM)
                        add_assoc_long(encaps, "rxcsum", 1);
                if (ifr.ifr_curcap & IFCAP_TXCSUM)
                        add_assoc_long(encaps, "txcsum", 1);
                if (ifr.ifr_curcap & IFCAP_VLAN_MTU)
                        add_assoc_long(encaps, "vlanmtu", 1);
                if (ifr.ifr_curcap & IFCAP_JUMBO_MTU)
                        add_assoc_long(encaps, "jumbomtu", 1);
                if (ifr.ifr_curcap & IFCAP_VLAN_HWTAGGING)
                        add_assoc_long(encaps, "vlanhwtag", 1);
                if (ifr.ifr_curcap & IFCAP_VLAN_HWCSUM)
                        add_assoc_long(encaps, "vlanhwcsum", 1);
                if (ifr.ifr_curcap & IFCAP_TSO4)
                        add_assoc_long(encaps, "tso4", 1);
                if (ifr.ifr_curcap & IFCAP_TSO6)
                        add_assoc_long(encaps, "tso6", 1);
                if (ifr.ifr_curcap & IFCAP_LRO)
                        add_assoc_long(encaps, "lro", 1);
                if (ifr.ifr_curcap & IFCAP_WOL_UCAST)
                        add_assoc_long(encaps, "wolucast", 1);
                if (ifr.ifr_curcap & IFCAP_WOL_MCAST)
                        add_assoc_long(encaps, "wolmcast", 1);
                if (ifr.ifr_curcap & IFCAP_WOL_MAGIC)
                        add_assoc_long(encaps, "wolmagic", 1);
                if (ifr.ifr_curcap & IFCAP_TOE4)
                        add_assoc_long(encaps, "toe4", 1);
                if (ifr.ifr_curcap & IFCAP_TOE6)
                        add_assoc_long(encaps, "toe6", 1);
                if (ifr.ifr_curcap & IFCAP_VLAN_HWFILTER)
			add_assoc_long(encaps, "vlanhwfilter", 1);
#if 0
                if (ifr.ifr_reqcap & IFCAP_POLLING_NOCOUNT)
                	add_assoc_long(caps, "pollingnocount", 1);
#endif
	}
	add_assoc_zval(return_value, "caps", caps);
	add_assoc_zval(return_value, "encaps", encaps);
	if (mb->ifa_addr == NULL)
		continue;
	switch (mb->ifa_addr->sa_family) {
	case AF_INET:
		if (addresscnt > 0)
			break;
                bzero(outputbuf, sizeof outputbuf);
                tmp = (struct sockaddr_in *)mb->ifa_addr;
                inet_ntop(AF_INET, (void *)&tmp->sin_addr, outputbuf, 128);
		add_assoc_string(return_value, "ipaddr", outputbuf, 1);
		addresscnt++;
                tmp = (struct sockaddr_in *)mb->ifa_netmask;
		unsigned char mask;
		const unsigned char *byte = (unsigned char *)&tmp->sin_addr.s_addr;
		int i = 0, n = sizeof(tmp->sin_addr.s_addr);
		while (n--) {
			mask = ((unsigned char)-1 >> 1) + 1;
				do {
					if (mask & byte[n])
						i++;
					mask >>= 1;
				} while (mask);
		}
		add_assoc_long(return_value, "subnetbits", i);

                bzero(outputbuf, sizeof outputbuf);
                inet_ntop(AF_INET, (void *)&tmp->sin_addr, outputbuf, 128);
                add_assoc_string(return_value, "subnet", outputbuf, 1);

                if (mb->ifa_flags & IFF_BROADCAST) {
                	bzero(outputbuf, sizeof outputbuf);
                                tmp = (struct sockaddr_in *)mb->ifa_broadaddr;
                                inet_ntop(AF_INET, (void *)&tmp->sin_addr, outputbuf, 128);
                                add_assoc_string(return_value, "broadcast", outputbuf, 1);
                        }

			if (mb->ifa_flags & IFF_POINTOPOINT) {
				bzero(outputbuf, sizeof outputbuf);
                                tmp = (struct sockaddr_in *)mb->ifa_dstaddr;
                                inet_ntop(AF_INET, (void *)&tmp->sin_addr, outputbuf, 128);
                                add_assoc_string(return_value, "tunnel", outputbuf, 1);
			}

		break;
		case AF_LINK:
                        tmpdl = (struct sockaddr_dl *)mb->ifa_addr;
                        bzero(outputbuf, sizeof outputbuf);
                        ether_ntoa_r((struct ether_addr *)LLADDR(tmpdl), outputbuf);
                        add_assoc_string(return_value, "macaddr", outputbuf, 1);
                        md = (struct if_data *)mb->ifa_data;

		break;
               }
        }
	freeifaddrs(ifdata);
}

PHP_FUNCTION(pfSense_bridge_add_member) {
	char *ifname, *ifchld;
        int ifname_len, ifchld_len;
	struct ifdrv drv;
	struct ifbreq req;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &ifchld, &ifchld_len) == FAILURE) {
                RETURN_NULL();
	}

	memset(&drv, 0, sizeof(drv));
	memset(&req, 0, sizeof(req));
	strlcpy(drv.ifd_name, ifname, sizeof(drv.ifd_name));
	strlcpy(req.ifbr_ifsname, ifchld, sizeof(req.ifbr_ifsname));
	drv.ifd_cmd = BRDGADD;
	drv.ifd_data = &req;
	drv.ifd_len = sizeof(req);
	if (ioctl(PFSENSE_G(s), SIOCSDRVSPEC, (caddr_t)&drv) < 0)
		RETURN_FALSE;

	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_bridge_del_member) {
	char *ifname, *ifchld;
        int ifname_len, ifchld_len;
	struct ifdrv drv;
	struct ifbreq req;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &ifchld, &ifchld_len) == FAILURE) {
                RETURN_NULL();
	}

	memset(&drv, 0, sizeof(drv));
	memset(&req, 0, sizeof(req));
	strlcpy(drv.ifd_name, ifname, sizeof(drv.ifd_name));
	strlcpy(req.ifbr_ifsname, ifchld, sizeof(req.ifbr_ifsname));
	drv.ifd_cmd = BRDGDEL;
	drv.ifd_data = &req;
	drv.ifd_len = sizeof(req);
	if (ioctl(PFSENSE_G(s), SIOCSDRVSPEC, (caddr_t)&drv) < 0)
		RETURN_FALSE;

	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_bridge_member_flags) {
	char *ifname, *ifchld;
        int ifname_len, ifchld_len;
	struct ifdrv drv;
	struct ifbreq req;
	int flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &ifname, &ifname_len, &ifchld, &ifchld_len, &flags) == FAILURE) {
                RETURN_NULL();
	}

	memset(&drv, 0, sizeof(drv));
	memset(&req, 0, sizeof(req));
	strlcpy(drv.ifd_name, ifname, sizeof(drv.ifd_name));
	strlcpy(req.ifbr_ifsname, ifchld, sizeof(req.ifbr_ifsname));
	drv.ifd_cmd = BRDGGIFFLGS;
	drv.ifd_data = &req;
	drv.ifd_len = sizeof(req);
	if (ioctl(PFSENSE_G(s), SIOCGDRVSPEC, (caddr_t)&drv) < 0)
		RETURN_FALSE;

	if (flags < 0) {
		flags = -flags;
		req.ifbr_ifsflags &= ~flags;
	} else
		req.ifbr_ifsflags |= flags;

	drv.ifd_cmd = BRDGSIFFLGS;
	drv.ifd_data = &req;
	drv.ifd_len = sizeof(req);
	if (ioctl(PFSENSE_G(s), SIOCSDRVSPEC, (caddr_t)&drv) < 0)
                RETURN_FALSE;

	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_listget) {
	struct ifaddrs *ifdata, *mb;
        char *ifname;
	int ifname_len;
	int flags = 0;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|l", &flags) == FAILURE)
                RETURN_NULL();

        getifaddrs(&ifdata);
        if (ifdata == NULL)
                RETURN_NULL();

	array_init(return_value);
	ifname = NULL;
	ifname_len = 0;
        for(mb = ifdata; mb != NULL; mb = mb->ifa_next) {
                if (mb == NULL)
                        continue;

		if (flags != 0) {
			if (mb->ifa_flags & IFF_UP && flags < 0)
				continue;
			if (!(mb->ifa_flags & IFF_UP) && flags > 0)
				continue;
		}

		if (ifname != NULL && ifname_len == strlen(mb->ifa_name) && strcmp(ifname, mb->ifa_name) == 0)
			continue;
		ifname = mb->ifa_name;
		ifname_len = strlen(mb->ifa_name);

		add_next_index_string(return_value, mb->ifa_name, 1);
	}

        freeifaddrs(ifdata);
}

PHP_FUNCTION(pfSense_interface_create) {
	char *ifname;
        int ifname_len, len;
	struct ifreq ifr;
	struct vlanreq params;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ifname, &ifname_len) == FAILURE) {
                RETURN_NULL();
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(PFSENSE_G(s), SIOCIFCREATE2, &ifr) < 0) {
		array_init(return_value);
		add_assoc_string(return_value, "error", "Could not create interface", 1);
	} else
		RETURN_STRING(ifr.ifr_name, 1)
}

PHP_FUNCTION(pfSense_interface_destroy) {
	char *ifname;
        int ifname_len;
	struct ifreq ifr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ifname, &ifname_len) == FAILURE) {
                RETURN_NULL();
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(PFSENSE_G(s), SIOCIFDESTROY, &ifr) < 0) {
		array_init(return_value);
		add_assoc_string(return_value, "error", "Could not create interface", 1);
	} else
		RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_setaddress) {
	char *ifname, *ip, *p = NULL;
        int ifname_len, ip_len;
	struct sockaddr_in *sin;
	struct in_aliasreq ifra;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &ip, &ip_len) == FAILURE) {
		RETURN_NULL();
	}

	memset(&ifra, 0, sizeof(ifra));
	strlcpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name));
	sin =  &ifra.ifra_mask;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	sin->sin_addr.s_addr = 0;
	if ((p = strrchr(ip, '/')) != NULL) {
		/* address is `name/masklen' */
		int masklen;
		int ret;
		*p = '\0';
		ret = sscanf(p+1, "%u", &masklen);
		if(ret != 1 || (masklen < 0 || masklen > 32)) {
			*p = '/';
			RETURN_FALSE;
		}
		sin->sin_addr.s_addr = htonl(~((1LL << (32 - masklen)) - 1) &
			0xffffffff);
	}
	sin =  &ifra.ifra_addr;
	sin->sin_family = AF_INET;
	sin->sin_len = sizeof(*sin);
	if (inet_pton(AF_INET, ip, &sin->sin_addr) <= 0)
		RETURN_FALSE;

	if (ioctl(PFSENSE_G(inets), SIOCAIFADDR, &ifra) < 0) {
		array_init(return_value);
		add_assoc_string(return_value, "error", "Could not set interface address", 1);
	} else
		RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_deladdress) {
        char *ifname, *ip = NULL;
        int ifname_len, ip_len;
        struct sockaddr_in *sin;
        struct in_aliasreq ifra;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &ip, &ip_len) == FAILURE) {
                RETURN_NULL();
        }

        memset(&ifra, 0, sizeof(ifra));
        strlcpy(ifra.ifra_name, ifname, sizeof(ifra.ifra_name));
        sin =  &ifra.ifra_addr;
        sin->sin_family = AF_INET;
        sin->sin_len = sizeof(*sin);
        if (inet_pton(AF_INET, ip, &sin->sin_addr) <= 0)
                RETURN_FALSE;

        if (ioctl(PFSENSE_G(inets), SIOCDIFADDR, &ifra) < 0) {
                array_init(return_value);
                add_assoc_string(return_value, "error", "Could not delete interface address", 1);
        } else
                RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_rename) {
	char *ifname, *newifname;
	int ifname_len, newifname_len;
	struct ifreq ifr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &newifname, &newifname_len) == FAILURE) {
                RETURN_NULL();
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	ifr.ifr_data = (caddr_t) newifname;
	if (ioctl(PFSENSE_G(s), SIOCSIFNAME, (caddr_t) &ifr) < 0) {
		array_init(return_value);
		add_assoc_string(return_value, "error", "Could not rename interface", 1);
	} else
		RETURN_TRUE;
}

PHP_FUNCTION(pfSense_ngctl_name) {
	char *ifname, *newifname;
	int ifname_len, newifname_len;

	if (PFSENSE_G(csock) == -1)
		RETURN_NULL();

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &newifname, &newifname_len) == FAILURE) {
                RETURN_NULL();
	}

	/* Send message */
	if (NgNameNode(PFSENSE_G(csock), ifname, newifname) < 0)
		RETURN_NULL();

	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_ngctl_attach) {
        char *ifname, *newifname;
        int ifname_len, newifname_len;
        struct ngm_name name;

	if (PFSENSE_G(csock) == -1)
		RETURN_NULL();

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &newifname, &newifname_len) == FAILURE) {
                RETURN_NULL();
        }

        snprintf(name.name, sizeof(name.name), "%s", newifname);
        /* Send message */
        if (NgSendMsg(PFSENSE_G(csock), ifname, NGM_GENERIC_COOKIE,
            NGM_ETHER_ATTACH, &name, sizeof(name)) < 0)
                RETURN_NULL();

        RETURN_TRUE;
}

PHP_FUNCTION(pfSense_ngctl_detach) {
        char *ifname, *newifname;
        int ifname_len, newifname_len;
        struct ngm_name name;

	if (PFSENSE_G(csock) == -1)
		RETURN_NULL();

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ifname, &ifname_len, &newifname, &newifname_len) == FAILURE) {
                RETURN_NULL();
        }

        snprintf(name.name, sizeof(name.name), "%s", newifname);
        /* Send message */
        if (NgSendMsg(PFSENSE_G(csock), ifname, NGM_ETHER_COOKIE,
            NGM_ETHER_DETACH, &name, sizeof(name)) < 0)
                RETURN_NULL();

        RETURN_TRUE;
}

PHP_FUNCTION(pfSense_vlan_create) {
	char *ifname = NULL;
	char *parentifname = NULL;
	int ifname_len, parent_len;
	int tag; 
	struct ifreq ifr;
	struct vlanreq params;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &ifname, &ifname_len, &parentifname, &parent_len, &tag) == FAILURE) {
                RETURN_NULL();
        }

	memset(&ifr, 0, sizeof(ifr));
	memset(&params, 0, sizeof(params));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	strlcpy(params.vlr_parent, parentifname, sizeof(params.vlr_parent));
	params.vlr_tag = (u_short) tag;
	ifr.ifr_data = (caddr_t) &params;
	if (ioctl(PFSENSE_G(s), SIOCSETVLAN, (caddr_t) &ifr) < 0)
		RETURN_NULL();

	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_mtu) {
	char *ifname;
	int ifname_len;
	int mtu;
	struct ifreq ifr;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &ifname, &ifname_len, &mtu) == FAILURE) {
                RETURN_NULL();
        }
	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = mtu;
	if (ioctl(PFSENSE_G(s), SIOCSIFMTU, (caddr_t)&ifr) < 0)
		RETURN_NULL();
	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_flags) {
	struct ifreq ifr;
	char *ifname;
	int flags, ifname_len, value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &ifname, &ifname_len, &value) == FAILURE) {
                RETURN_NULL();
        }

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(PFSENSE_G(s), SIOCGIFFLAGS, (caddr_t)&ifr) < 0) {
		RETURN_NULL();
	}
	flags = (ifr.ifr_flags & 0xffff) | (ifr.ifr_flagshigh << 16);
	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value; 
	ifr.ifr_flags = flags & 0xffff;
	ifr.ifr_flagshigh = flags >> 16;
	if (ioctl(PFSENSE_G(s), SIOCSIFFLAGS, (caddr_t)&ifr) < 0)
		RETURN_NULL();
	RETURN_TRUE;
}

PHP_FUNCTION(pfSense_interface_capabilities) {
	struct ifreq ifr;
	char *ifname;
	int flags, ifname_len, value;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &ifname, &ifname_len, &value) == FAILURE) {
                RETURN_NULL();
        }

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
	if (ioctl(PFSENSE_G(s), SIOCGIFCAP, (caddr_t)&ifr) < 0) {
		RETURN_NULL();
	}
	flags = ifr.ifr_curcap;
	if (value < 0) {
		value = -value;
		flags &= ~value;
	} else
		flags |= value; 
	flags &= ifr.ifr_reqcap;
	ifr.ifr_reqcap = flags;
	if (ioctl(PFSENSE_G(s), SIOCSIFCAP, (caddr_t)&ifr) < 0)
		RETURN_NULL();
	RETURN_TRUE;

}

PHP_FUNCTION(pfSense_get_interface_info)
{
	struct ifaddrs *ifdata, *mb;
	struct if_data *tmpd;
        struct sockaddr_in *tmp;
        struct sockaddr_dl *tmpdl;
	struct pfi_kif kif = { 0 };
	int size = 1, found = 0;
        char outputbuf[128];
        char *ifname;
        int ifname_len;
        int i = 0, error = 0;
	int dev;
	char *pf_status;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ifname, &ifname_len) == FAILURE) {
                RETURN_NULL();
        }

	if ((dev = open("/dev/pf", O_RDWR)) < 0)
		RETURN_NULL();

        getifaddrs(&ifdata);
        if (ifdata == NULL) {
		close(dev);
		RETURN_NULL();
        }

        for(mb = ifdata; mb != NULL; mb = mb->ifa_next) {
                if (mb == NULL)
                        continue;
		if (ifname_len != strlen(mb->ifa_name))
                        continue;
                if (strncmp(ifname, mb->ifa_name, ifname_len) != 0)
                        continue;

		if (found == 0)
			array_init(return_value);

		found = 1;

                switch (mb->ifa_addr->sa_family) {
                case AF_LINK:

                        tmpd = (struct if_data *)mb->ifa_data;
			add_assoc_long(return_value, "inerrs", tmpd->ifi_ierrors);
			add_assoc_long(return_value, "outerrs", tmpd->ifi_oerrors);
			add_assoc_long(return_value, "collisions", tmpd->ifi_collisions);
			add_assoc_long(return_value, "inmcasts", tmpd->ifi_imcasts);
			add_assoc_long(return_value, "outmcasts", tmpd->ifi_omcasts);
			add_assoc_long(return_value, "unsuppproto", tmpd->ifi_noproto);
			add_assoc_long(return_value, "mtu", tmpd->ifi_mtu);

                        break;
                }
        }
        freeifaddrs(ifdata);

	if (found == 0) {
		close(dev);
		RETURN_NULL();
	}

	if (pfi_get_ifaces(dev, ifname, &kif, &size))
		error = 1;

	if (error == 0) {
                add_assoc_string(return_value, "interface", kif.pfik_name, 1);

#define PAF_INET 0
#define PPF_IN 0
#define PPF_OUT 1
                add_assoc_long(return_value, "inpktspass", (unsigned long long)kif.pfik_packets[PAF_INET][PPF_IN][PF_PASS]);
                add_assoc_long(return_value, "outpktspass", (unsigned long long)kif.pfik_packets[PAF_INET][PPF_OUT][PF_PASS]);
                add_assoc_long(return_value, "inbytespass", (unsigned long long)kif.pfik_bytes[PAF_INET][PPF_IN][PF_PASS]);
                add_assoc_long(return_value, "outbytespass", (unsigned long long)kif.pfik_bytes[PAF_INET][PPF_OUT][PF_PASS]);

                add_assoc_long(return_value, "inpktsblock", (unsigned long long)kif.pfik_packets[PAF_INET][PPF_IN][PF_DROP]);
                add_assoc_long(return_value, "outpktsblock", (unsigned long long)kif.pfik_packets[PAF_INET][PPF_OUT][PF_DROP]);
                add_assoc_long(return_value, "inbytesblock", (unsigned long long)kif.pfik_bytes[PAF_INET][PPF_IN][PF_DROP]);
                add_assoc_long(return_value, "outbytesblock", (unsigned long long)kif.pfik_bytes[PAF_INET][PPF_OUT][PF_DROP]);

                add_assoc_long(return_value, "inbytes", (unsigned long long)(kif.pfik_bytes[PAF_INET][PPF_IN][PF_DROP] + kif.pfik_bytes[PAF_INET][PPF_IN][PF_PASS]));
                add_assoc_long(return_value, "outbytes", (unsigned long long)(kif.pfik_bytes[PAF_INET][PPF_OUT][PF_DROP] + kif.pfik_bytes[PAF_INET][PPF_OUT][PF_PASS]));
                add_assoc_long(return_value, "inpkts", (unsigned long long)(kif.pfik_packets[PAF_INET][PPF_IN][PF_DROP] + kif.pfik_packets[PAF_INET][PPF_IN][PF_PASS]));
                add_assoc_long(return_value, "outpkts", (unsigned long long)(kif.pfik_packets[PAF_INET][PPF_OUT][PF_DROP] + kif.pfik_packets[PAF_INET][PPF_OUT][PF_PASS]));
#undef PPF_IN
#undef PPF_OUT
#undef PAF_INET
        }
	close(dev);
}

PHP_FUNCTION(pfSense_get_interface_stats)
{
	struct ifmibdata ifmd;
	struct if_data *tmpd;
        char outputbuf[128];
        char *ifname;
        int ifname_len, error;
	int name[6];
	size_t len;
	unsigned int ifidx;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ifname, &ifname_len) == FAILURE) {
                RETURN_NULL();
        }

	ifidx = if_nametoindex(ifname);
	if (ifidx == 0)
		RETURN_NULL();

	name[0] = CTL_NET;
	name[1] = PF_LINK;
	name[2] = NETLINK_GENERIC;
	name[3] = IFMIB_IFDATA;
	name[4] = ifidx;
	name[5] = IFDATA_GENERAL;

	len = sizeof(ifmd);

	if (sysctl(name, 6, &ifmd, &len, (void *)0, 0) < 0)
		RETURN_NULL();
	
	tmpd = &ifmd.ifmd_data;

        array_init(return_value);
	add_assoc_long(return_value, "inpkts", (long)tmpd->ifi_ipackets);
	add_assoc_long(return_value, "inbytes", (long)tmpd->ifi_ibytes);
	add_assoc_long(return_value, "outpkts", (long)tmpd->ifi_opackets);
	add_assoc_long(return_value, "outbytes", (long)tmpd->ifi_obytes);
	add_assoc_long(return_value, "inerrs", (long)tmpd->ifi_ierrors);
	add_assoc_long(return_value, "outerrs", (long)tmpd->ifi_oerrors);
	add_assoc_long(return_value, "collisions", (long)tmpd->ifi_collisions);
	add_assoc_long(return_value, "inmcasts", (long)tmpd->ifi_imcasts);
	add_assoc_long(return_value, "outmcasts", (long)tmpd->ifi_omcasts);
	add_assoc_long(return_value, "unsuppproto", (long)tmpd->ifi_noproto);
	add_assoc_long(return_value, "mtu", (long)tmpd->ifi_mtu);
}

PHP_FUNCTION(pfSense_get_pf_stats) {
	struct pf_status status;
	time_t runtime;
	unsigned sec, min, hrs, day = runtime;
	char statline[80];
	char buf[PF_MD5_DIGEST_LENGTH * 2 + 1];
	static const char hex[] = "0123456789abcdef";
	int i;
	int dev;

	array_init(return_value);

	if ((dev = open("/dev/pf", O_RDWR)) < 0) {
		add_assoc_string(return_value, "error", strerror(errno), 1);
	} else {


	bzero(&status, sizeof(status));
        if (ioctl(dev, DIOCGETSTATUS, &status)) {
		add_assoc_string(return_value, "error", strerror(errno), 1);
	} else {
                add_assoc_long(return_value, "rulesmatch", (unsigned long long)status.counters[PFRES_MATCH]);
                add_assoc_long(return_value, "pullhdrfail", (unsigned long long)status.counters[PFRES_BADOFF]);
                add_assoc_long(return_value, "fragments", (unsigned long long)status.counters[PFRES_FRAG]);
                add_assoc_long(return_value, "shortpacket", (unsigned long long)status.counters[PFRES_SHORT]);
                add_assoc_long(return_value, "normalizedrop", (unsigned long long)status.counters[PFRES_NORM]);
                add_assoc_long(return_value, "nomemory", (unsigned long long)status.counters[PFRES_MEMORY]);
                add_assoc_long(return_value, "badtimestamp", (unsigned long long)status.counters[PFRES_TS]);
                add_assoc_long(return_value, "congestion", (unsigned long long)status.counters[PFRES_CONGEST]);
                add_assoc_long(return_value, "ipoptions", (unsigned long long)status.counters[PFRES_IPOPTIONS]);
                add_assoc_long(return_value, "protocksumbad", (unsigned long long)status.counters[PFRES_PROTCKSUM]);
                add_assoc_long(return_value, "statesbad", (unsigned long long)status.counters[PFRES_BADSTATE]);
                add_assoc_long(return_value, "stateinsertions", (unsigned long long)status.counters[PFRES_STATEINS]);
                add_assoc_long(return_value, "maxstatesdrop", (unsigned long long)status.counters[PFRES_MAXSTATES]);
                add_assoc_long(return_value, "srclimitdrop", (unsigned long long)status.counters[PFRES_SRCLIMIT]);
                add_assoc_long(return_value, "synproxydrop", (unsigned long long)status.counters[PFRES_SYNPROXY]);

                add_assoc_long(return_value, "maxstatesreached", (unsigned long long)status.lcounters[LCNT_STATES]);
                add_assoc_long(return_value, "maxsrcstatesreached", (unsigned long long)status.lcounters[LCNT_SRCSTATES]);
                add_assoc_long(return_value, "maxsrcnodesreached", (unsigned long long)status.lcounters[LCNT_SRCNODES]);
                add_assoc_long(return_value, "maxsrcconnreached", (unsigned long long)status.lcounters[LCNT_SRCCONN]);
                add_assoc_long(return_value, "maxsrcconnratereached", (unsigned long long)status.lcounters[LCNT_SRCCONNRATE]);
                add_assoc_long(return_value, "overloadtable", (unsigned long long)status.lcounters[LCNT_OVERLOAD_TABLE]);
                add_assoc_long(return_value, "overloadflush", (unsigned long long)status.lcounters[LCNT_OVERLOAD_FLUSH]);

                add_assoc_long(return_value, "statesearch", (unsigned long long)status.fcounters[FCNT_STATE_SEARCH]);
                add_assoc_long(return_value, "stateinsert", (unsigned long long)status.fcounters[FCNT_STATE_INSERT]);
                add_assoc_long(return_value, "stateremovals", (unsigned long long)status.fcounters[FCNT_STATE_REMOVALS]);

                add_assoc_long(return_value, "srcnodessearch", (unsigned long long)status.scounters[SCNT_SRC_NODE_SEARCH]);
                add_assoc_long(return_value, "srcnodesinsert", (unsigned long long)status.scounters[SCNT_SRC_NODE_INSERT]);
                add_assoc_long(return_value, "srcnodesremovals", (unsigned long long)status.scounters[SCNT_SRC_NODE_REMOVALS]);

                add_assoc_long(return_value, "stateid", (unsigned long long)status.stateid);

                add_assoc_long(return_value, "running", status.running);
                add_assoc_long(return_value, "states", status.states);
                add_assoc_long(return_value, "srcnodes", status.src_nodes);

                add_assoc_long(return_value, "hostid", ntohl(status.hostid));
		for (i = 0; i < PF_MD5_DIGEST_LENGTH; i++) {
			buf[i + i] = hex[status.pf_chksum[i] >> 4];
			buf[i + i + 1] = hex[status.pf_chksum[i] & 0x0f];
		}
		buf[i + i] = '\0';
		add_assoc_string(return_value, "pfchecksum", buf, 1);
		printf("Checksum: 0x%s\n\n", buf);

		switch(status.debug) {
		case PF_DEBUG_NONE:
			add_assoc_string(return_value, "debuglevel", "none", 1);
			break;
		case PF_DEBUG_URGENT:
			add_assoc_string(return_value, "debuglevel", "urgent", 1);
			break;
		case PF_DEBUG_MISC:
			add_assoc_string(return_value, "debuglevel", "misc", 1);
			break;
		case PF_DEBUG_NOISY:
			add_assoc_string(return_value, "debuglevel", "noisy", 1);
			break;
		default:
			add_assoc_string(return_value, "debuglevel", "unknown", 1);
			break;
		}

		runtime = time(NULL) - status.since;
		if (status.since) {
			sec = day % 60;
			day /= 60;
			min = day % 60;
			day /= 60;
			hrs = day % 24;
			day /= 24;
			snprintf(statline, sizeof(statline),
		    		"Running: for %u days %.2u:%.2u:%.2u",
		    		day, hrs, min, sec);
			add_assoc_string(return_value, "uptime", statline, 1);
		}
	}
	close(dev);
	}
}

#define PATH_LOCKFILENAME     "/var/spool/lock/LCK..%s"
#define MAX_FILENAME              1000
#define MAX_OPEN_DELAY    2
#define MODEM_DEFAULT_SPEED              115200

static int
ExclusiveOpenDevice(const char *pathname)
{
	int           fd;
	const char    *ttyname = NULL;
	time_t        startTime;

	/* Open it, but give up after so many interruptions */
	for (startTime = time(NULL);
	    (fd = open(pathname, O_RDWR, 0)) < 0
	    && time(NULL) < startTime + MAX_OPEN_DELAY; )
		if (errno != EINTR)
			return(-1);

	/* Done */
	return(fd);
}

static void
ExclusiveCloseDevice(int fd, const char *pathname)
{
	int           rtn = -1;
	const char    *ttyname;
	time_t        startTime;

	/* Close file(s) */
	for (startTime = time(NULL);
	    time(NULL) < startTime + MAX_OPEN_DELAY &&
	    (rtn = close(fd)) < 0; )
		if (errno != EINTR)
			break;

	/* Did we succeed? */
	if ((rtn < 0) && (errno == EINTR))
		return;
}

PHP_FUNCTION(pfSense_sync) {
	sync();
}

PHP_FUNCTION(pfSense_get_modem_devices) {
	struct termios		attr;
	struct pollfd		pfd;
	glob_t			g;
	char			buf[2048] = { 0 };
	char			*path;
	int			nw = 0, i, fd;
	zend_bool		show_info = 0;
	int			poll_timeout = 700;

	if (ZEND_NUM_ARGS() > 2) {
		RETURN_NULL();
	}
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|bl", &show_info, &poll_timeout) == FAILURE) {
		php_printf("Maximum two parameter can be passed\n");
                RETURN_NULL();
        }

	array_init(return_value);

	bzero(&g, sizeof g);
	glob("/dev/cua*", 0, NULL, &g);
        glob("/dev/modem*", GLOB_APPEND, NULL, &g);

	if (g.gl_pathc > 0)
	for (i = 0; g.gl_pathv[i] != NULL; i++) {
		path = g.gl_pathv[i];
		if (strstr(path, "lock") || strstr(path, "init"))
			continue;
		if (show_info)
			php_printf("Found modem device: %s\n", path);
		/* Open & lock serial port */
		if ((fd = ExclusiveOpenDevice(path)) < 0) {
			if (show_info)
				php_printf("Could not open the device exlusively\n");
			add_assoc_string(return_value, path, path, 1);
			continue;
		}

		/* Set non-blocking I/O  */
		if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
			goto errormodem;

		/* Set serial port raw mode, baud rate, hardware flow control, etc. */
		if (tcgetattr(fd, &attr) < 0)
			goto errormodem;

		cfmakeraw(&attr);

		attr.c_cflag &= ~(CSIZE|PARENB|PARODD);
		attr.c_cflag |= (CS8|CREAD|CLOCAL|HUPCL|CCTS_OFLOW|CRTS_IFLOW);
		attr.c_iflag &= ~(IXANY|IMAXBEL|ISTRIP|IXON|IXOFF|BRKINT|ICRNL|INLCR);
		attr.c_iflag |= (IGNBRK|IGNPAR);
		attr.c_oflag &= ~OPOST;
		attr.c_lflag = 0;

		cfsetspeed(&attr, (speed_t) MODEM_DEFAULT_SPEED);

		if (tcsetattr(fd, TCSANOW, &attr) < 0)
			goto errormodem;

		/* OK */
tryagain:
		if ((nw = write(fd, "AT OK\r\n", strlen("AT OK\r\n"))) < 0) {
			if (errno == EAGAIN) {
				if (show_info)
					php_printf("\tRetrying write\n");
				goto tryagain;
			}

			if (show_info)
				php_printf("\tError ocurred\n");
			goto errormodem;
		}

tryagain2:
		if (show_info)
			php_printf("\tTrying to read data\n");
		bzero(buf, sizeof buf);
		bzero(&pfd, sizeof pfd);
		pfd.fd = fd;
		pfd.events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLHUP;
		if ((nw = poll(&pfd, 1, poll_timeout)) > 0) {
			if ((nw = read(fd, buf, sizeof(buf))) < 0) {
				if (errno == EAGAIN) {
					if (show_info)
						php_printf("\tTrying again after errno = EAGAIN\n");
					goto tryagain2;
				}
				if (show_info)
					php_printf("\tError ocurred on 1st read\n");
				goto errormodem;
			}

			buf[2047] = '\0';
			if (show_info)
				php_printf("\tRead %s\n", buf);
			//if (strnstr(buf, "OK", sizeof(buf))) {
			if (nw > 0) {
				/*
				write(fd, "ATI3\r\n", strlen("ATI3\r\n"));
				bzero(buf, sizeof buf);
                		bzero(&pfd, sizeof pfd);
                		pfd.fd = fd;
                		pfd.events = POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI | POLLHUP;

                		if (poll(&pfd, 1, 200) > 0) {
					read(fd, buf, sizeof(buf));
				buf[2047] = '\0';
				if (show_info)
					php_printf("\tRead %s\n", buf);
				}
				*/
				add_assoc_string(return_value, path, path, 1);
			}
		} else if (show_info)
			php_printf("\ttimedout or interrupted: %d\n", nw);

errormodem:
		if (show_info)
			php_printf("\tClosing device %s\n", path);
		ExclusiveCloseDevice(fd, path);
	}
}

PHP_FUNCTION(pfSense_get_os_hw_data) {
	int i, mib[4], idata;
	size_t len;	
	char *data;

	array_init(return_value);

	mib[0] = CTL_HW;
	mib[1] = HW_MACHINE;
	if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
		data = malloc(len);
		if (data != NULL) {
			if (!sysctl(mib, 2, data, &len, NULL, 0)) {
				add_assoc_string(return_value, "hwmachine", data, 1);
				free(data);
			}
		}
	}

	mib[0] = CTL_HW;
        mib[1] = HW_MODEL;
        if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
                data = malloc(len);
                if (data != NULL) {
                        if (!sysctl(mib, 2, data, &len, NULL, 0)) {
                                add_assoc_string(return_value, "hwmodel", data, 1);
                                free(data);
                        }
		}
        }

	mib[0] = CTL_HW;
        mib[1] = HW_MACHINE_ARCH;
        if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
                data = malloc(len);
                if (data != NULL) {
                        if (!sysctl(mib, 2, data, &len, NULL, 0)) {
                                add_assoc_string(return_value, "hwarch", data, 1);
                                free(data);
                        }
                }
        }

	mib[0] = CTL_HW;
        mib[1] = HW_NCPU;
	len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
		add_assoc_long(return_value, "ncpus", idata);

	mib[0] = CTL_HW;
        mib[1] = HW_PHYSMEM;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "physmem", idata);

	mib[0] = CTL_HW;
        mib[1] = HW_USERMEM;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "usermem", idata);

	mib[0] = CTL_HW;
        mib[1] = HW_REALMEM;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "realmem", idata);
}

PHP_FUNCTION(pfSense_get_os_kern_data) {
        int i, mib[4], idata;
        size_t len;
        char *data;
	struct timeval bootime;

	array_init(return_value);

        mib[0] = CTL_KERN;
        mib[1] = KERN_HOSTUUID;
        if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
                data = malloc(len);
                if (data != NULL) {
                        if (!sysctl(mib, 2, data, &len, NULL, 0)) {
                                add_assoc_string(return_value, "hostuuid", data, 1);
                                free(data);
                        }
                }
        }

        mib[0] = CTL_KERN;
        mib[1] = KERN_HOSTNAME;
        if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
                data = malloc(len);
                if (data != NULL) {
                        if (!sysctl(mib, 2, data, &len, NULL, 0)) {
                                add_assoc_string(return_value, "hostname", data, 1);
                                free(data);
                        }
                }
        }

        mib[0] = CTL_KERN;
        mib[1] = KERN_OSRELEASE;
        if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
                data = malloc(len);
                if (data != NULL) {
                        if (!sysctl(mib, 2, data, &len, NULL, 0)) {
                                add_assoc_string(return_value, "osrelease", data, 1);
                                free(data);
                        }
                }
        }

	mib[0] = CTL_KERN;
        mib[1] = KERN_VERSION;
        if (!sysctl(mib, 2, NULL, &len, NULL, 0)) {
                data = malloc(len);
                if (data != NULL) {
                        if (!sysctl(mib, 2, data, &len, NULL, 0)) {
                                add_assoc_string(return_value, "oskernel_version", data, 1);
                                free(data);
                        }
                }
        }

        mib[0] = CTL_KERN;
        mib[1] = KERN_BOOTTIME;
        len = sizeof(bootime);
        if (!sysctl(mib, 2, &bootime, &len, NULL, 0))
                add_assoc_string(return_value, "boottime", ctime(&bootime.tv_sec), 1);

        mib[0] = CTL_KERN;
        mib[1] = KERN_HOSTID;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "hostid", idata);

        mib[0] = CTL_KERN;
        mib[1] = KERN_OSRELDATE;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "osreleasedate", idata);

	mib[0] = CTL_KERN;
        mib[1] = KERN_OSREV;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "osrevision", idata);

	mib[0] = CTL_KERN;
        mib[1] = KERN_SECURELVL;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "ossecurelevel", idata);

	mib[0] = CTL_KERN;
        mib[1] = KERN_OSRELDATE;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "osreleasedate", idata);

	mib[0] = CTL_KERN;
        mib[1] = KERN_OSRELDATE;
        len = sizeof(idata);
        if (!sysctl(mib, 2, &idata, &len, NULL, 0))
                add_assoc_long(return_value, "osreleasedate", idata);
}
