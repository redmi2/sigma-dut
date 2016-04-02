/*
 * Sigma Control API DUT (station/AP)
 * Copyright (c) 2010-2011, Atheros Communications, Inc.
 * Copyright (c) 2011-2015, Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Licensed under the Clear BSD license. See README for more details.
 */

#ifndef SIGMA_DUT_H
#define SIGMA_DUT_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <net/if.h>
#ifdef __QNXNTO__
#include <sys/select.h>
#include <net/if_ether.h>
#endif /* __QNXNTO__ */
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef CONFIG_TRAFFIC_AGENT
#include <pthread.h>
#endif /* CONFIG_TRAFFIC_AGENT */


#ifdef __GNUC__
#define PRINTF_FORMAT(a,b) __attribute__ ((format (printf, (a), (b))))
#else
#define PRINTF_FORMAT(a,b)
#endif

#ifndef SIGMA_TMPDIR
#define SIGMA_TMPDIR "/tmp"
#endif /* SIGMA_TMPDIR */

#ifndef SIGMA_DUT_VER
#define SIGMA_DUT_VER "(unknown)"
#endif /* SIGMA_DUT_VER */

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

#ifndef ETH_P_ARP
#define ETH_P_ARP 0x0806
#endif

struct sigma_dut;

#define MAX_PARAMS 100
#define MAX_RADIO 2

/* Set default operating channel width 80 MHz */
#define VHT_DEFAULT_OPER_CHWIDTH AP_80_VHT_OPER_CHWIDTH

typedef unsigned int u32;
typedef unsigned char u8;

#define WPA_GET_BE32(a) ((((u32) (a)[0]) << 24) | (((u32) (a)[1]) << 16) | \
			 (((u32) (a)[2]) << 8) | ((u32) (a)[3]))
#define WPA_PUT_BE32(a, val)					\
	do {							\
		(a)[0] = (u8) ((((u32) (val)) >> 24) & 0xff);	\
		(a)[1] = (u8) ((((u32) (val)) >> 16) & 0xff);	\
		(a)[2] = (u8) ((((u32) (val)) >> 8) & 0xff);	\
		(a)[3] = (u8) (((u32) (val)) & 0xff);		\
	} while (0)

struct sigma_cmd {
	char *params[MAX_PARAMS];
	char *values[MAX_PARAMS];
	int count;
};

#define MAX_CMD_LEN 2048

struct sigma_conn {
	int s;
	struct sockaddr_in addr;
	socklen_t addrlen;
	char buf[MAX_CMD_LEN + 5];
	int pos;
	int waiting_completion;
};

#define SIGMA_DUT_ERROR_CALLER_SEND_STATUS -2
#define SIGMA_DUT_INVALID_CALLER_SEND_STATUS -1
#define SIGMA_DUT_SUCCESS_STATUS_SENT 0
#define SIGMA_DUT_SUCCESS_CALLER_SEND_STATUS 1

struct sigma_cmd_handler {
	struct sigma_cmd_handler *next;
	char *cmd;
	int (*validate)(struct sigma_cmd *cmd);
	/* process return value:
	 * -2 = failed, caller will send status,ERROR
	 * -1 = failed, caller will send status,INVALID
	 * 0 = success, response already sent
	 * 1 = success, caller will send status,COMPLETE
	 */
	int (*process)(struct sigma_dut *dut, struct sigma_conn *conn,
		       struct sigma_cmd *cmd);
};

#define P2P_GRP_ID_LEN 128
#define IP_ADDR_STR_LEN 16

struct wfa_cs_p2p_group {
	struct wfa_cs_p2p_group *next;
	char ifname[IFNAMSIZ];
	int go;
	char grpid[P2P_GRP_ID_LEN];
	char ssid[33];
};

#ifdef CONFIG_TRAFFIC_AGENT

#define MAX_SIGMA_STREAMS 16
#define MAX_SIGMA_STATS 6000

struct sigma_frame_stats {
	unsigned int seqnum;
	unsigned int local_sec;
	unsigned int local_usec;
	unsigned int remote_sec;
	unsigned int remote_usec;
};

struct sigma_stream {
	enum sigma_stream_profile {
		SIGMA_PROFILE_FILE_TRANSFER,
		SIGMA_PROFILE_MULTICAST,
		SIGMA_PROFILE_IPTV,
		SIGMA_PROFILE_TRANSACTION,
		SIGMA_PROFILE_START_SYNC,
		SIGMA_PROFILE_UAPSD
	} profile;
	int sender;
	struct in_addr dst;
	int dst_port;
	struct in_addr src;
	int src_port;
	int frame_rate;
	int duration;
	int payload_size;
	int start_delay;
	int max_cnt;
	enum sigma_traffic_class {
		SIGMA_TC_VOICE,
		SIGMA_TC_VIDEO,
		SIGMA_TC_BACKGROUND,
		SIGMA_TC_BEST_EFFORT
	} tc;
	int user_priority;
	int user_priority_set;
	int started;
	int no_timestamps;

	int sock;
	pthread_t thr;
	int stop;
	int ta_send_in_progress;
	int trans_proto;

	/* Statistics */
	int tx_act_frames; /*
			    * Number of frames generated by the traffic
			    * generator application. The name is defined in the
			    * Sigma CAPI spec.
			    */
	int tx_frames;
	int rx_frames;
	unsigned long long tx_payload_bytes;
	unsigned long long rx_payload_bytes;
	int out_of_seq_frames;
	struct sigma_frame_stats *stats;
	unsigned int num_stats;
	unsigned int stream_id;

	/* U-APSD */
	unsigned int sta_id;
	unsigned int rx_cookie;
	unsigned int uapsd_sta_tc;
	unsigned int uapsd_rx_state;
	unsigned int uapsd_tx_state;
	unsigned int tx_stop_cnt;
	unsigned int tx_hello_cnt;
	pthread_t uapsd_send_thr;
	pthread_cond_t tx_thr_cond;
	pthread_mutex_t tx_thr_mutex;
	int reset_rx;
	int num_retry;
	char ifname[IFNAMSIZ]; /* ifname from the command */
	struct sigma_dut *dut; /* for traffic agent thread to access context */
	/* console */
	char test_name[9]; /* test case name */
	int can_quit;
	int reset;
};

#endif /* CONFIG_TRAFFIC_AGENT */


#define NUM_AP_AC 4
#define AP_AC_BE 0
#define AP_AC_BK 1
#define AP_AC_VI 2
#define AP_AC_VO 3

struct sigma_dut {
	int s; /* server TCP socket */
	int debug_level;
	int stdout_debug;
	struct sigma_cmd_handler *cmds;

	/* Default timeout value (seconds) for commands */
	unsigned int default_timeout;

	int next_streamid;

	const char *bridge; /* bridge interface to use in AP mode */

	enum sigma_mode {
		SIGMA_MODE_UNKNOWN,
		SIGMA_MODE_STATION,
		SIGMA_MODE_AP,
		SIGMA_MODE_SNIFFER
	} mode;

	/*
	 * Local cached values to handle API that does not provide all the
	 * needed information with commands that actually trigger some
	 * operations.
	 */
	int listen_chn;
	int persistent;
	int intra_bss;
	int noa_duration;
	int noa_interval;
	int noa_count;
	enum wfa_cs_wps_method {
		WFA_CS_WPS_NOT_READY,
		WFA_CS_WPS_PBC,
		WFA_CS_WPS_PIN_DISPLAY,
		WFA_CS_WPS_PIN_LABEL,
		WFA_CS_WPS_PIN_KEYPAD
	} wps_method;
	char wps_pin[9];

	struct wfa_cs_p2p_group *groups;

	char infra_ssid[33];
	int infra_network_id;

	enum p2p_mode {
		P2P_IDLE, P2P_DISCOVER, P2P_LISTEN, P2P_DISABLE
	} p2p_mode;

	int go;
	int p2p_client;

	int client_uapsd;

	char arp_ipaddr[IP_ADDR_STR_LEN];
	char arp_ifname[IFNAMSIZ + 1];

	enum sta_pmf {
		STA_PMF_DISABLED,
		STA_PMF_OPTIONAL,
		STA_PMF_REQUIRED
	} sta_pmf;

	int no_tpk_expiration;

	int er_oper_performed;
	char er_oper_bssid[20];
	int amsdu_size;
	int back_rcv_buf;

	int testbed_flag_txsp;
	int testbed_flag_rxsp;
	int chwidth;

	/* AP configuration */
	char ap_ssid[33];
	char ap2_ssid[33];
	enum ap_mode {
		AP_11a,
		AP_11g,
		AP_11b,
		AP_11na,
		AP_11ng,
		AP_11ac,
		AP_inval
	} ap_mode;
	int ap_channel;
	int ap_rts;
	int ap_frgmnt;
	int ap_bcnint;
	struct qos_params {
		int ac;
		int cwmin;
		int cwmax;
		int aifs;
		int txop;
		int acm;
	} ap_qos[NUM_AP_AC], ap_sta_qos[NUM_AP_AC];
	enum ap_noack_values {
		AP_NOACK_NOT_SET,
		AP_NOACK_ENABLED,
		AP_NOACK_DISABLED
	} ap_noack;
	int ap_ampdu;
	int ap_amsdu;
	int ap_rx_amsdu;
	int ap_ampdu_exp;
	int ap_addba_reject;
	int ap_fixed_rate;
	int ap_mcs;
	int ap_rx_streams;
	int ap_tx_streams;
	unsigned int ap_vhtmcs_map;
	int ap_ldpc;
	int ap_sig_rts;
	enum ap_chwidth {
		AP_20,
		AP_40,
		AP_80,
		AP_160,
		AP_AUTO
	} ap_chwidth;
	enum ap_chwidth default_ap_chwidth;
	int ap_tx_stbc;
	enum ap_dyn_bw_sig_values {
		AP_DYN_BW_SGNL_NOT_SET,
		AP_DYN_BW_SGNL_ENABLED,
		AP_DYN_BW_SGNL_DISABLED
	} ap_dyn_bw_sig;
	int ap_sgi80;
	int ap_p2p_mgmt;
	enum ap_key_mgmt {
		AP_OPEN,
		AP_WPA2_PSK,
		AP_WPA_PSK,
		AP_WPA2_EAP,
		AP_WPA_EAP,
		AP_WPA2_EAP_MIXED,
		AP_WPA2_PSK_MIXED
	} ap_key_mgmt;
	enum ap2_key_mgmt {
		AP2_OPEN,
		AP2_OSEN
	} ap2_key_mgmt;
	int ap_add_sha256;
	int ap_rsn_preauth;
	enum ap_pmf {
		AP_PMF_DISABLED,
		AP_PMF_OPTIONAL,
		AP_PMF_REQUIRED
	} ap_pmf;
	enum ap_cipher {
		AP_CCMP,
		AP_TKIP,
		AP_WEP,
		AP_PLAIN,
		AP_CCMP_TKIP
	} ap_cipher;
	char ap_passphrase[65];
	char ap_wepkey[27];
	char ap_radius_ipaddr[20];
	int ap_radius_port;
	char ap_radius_password[200];
	char ap2_radius_ipaddr[20];
	int ap2_radius_port;
	char ap2_radius_password[200];
	int ap_tdls_prohibit;
	int ap_tdls_prohibit_chswitch;
	int ap_hs2;
	int ap_dgaf_disable;
	int ap_p2p_cross_connect;
	int ap_oper_name;
	int ap_wan_metrics;
	int ap_conn_capab;
	int ap_oper_class;

	int ap_interworking;
	int ap_access_net_type;
	int ap_internet;
	int ap_venue_group;
	int ap_venue_type;
	char ap_hessid[20];
	char ap_roaming_cons[100];
	int ap_venue_name;
	int ap_net_auth_type;
	int ap_nai_realm_list;
	char ap_domain_name_list[1000];
	int ap_ip_addr_type_avail;
	char ap_plmn_mcc[10][4];
	char ap_plmn_mnc[10][4];
	int ap_gas_cb_delay;
	int ap_proxy_arp;
	int ap2_proxy_arp;
	int ap_l2tif;
	int ap_anqpserver;
	int ap_anqpserver_on;
	int ap_osu_provider_list;
	int ap_qos_map_set;
	int ap_bss_load;
	char ap_osu_server_uri[10][256];
	char ap_osu_ssid[33];
	int ap_osu_method[10];
	int ap_osu_icon_tag;

	int ap_fake_pkhash;
	int ap_disable_protection;
	int ap_allow_vht_wep;
	int ap_allow_vht_tkip;

	enum ap_vht_chwidth {
		AP_20_40_VHT_OPER_CHWIDTH,
		AP_80_VHT_OPER_CHWIDTH,
		AP_160_VHT_OPER_CHWIDTH
	} ap_vht_chwidth;
	int ap_txBF;
	int ap_mu_txBF;
	enum ap_regulatory_mode {
		AP_80211D_MODE_DISABLED,
		AP_80211D_MODE_ENABLED,
	} ap_regulatory_mode;
	enum ap_dfs_mode {
		AP_DFS_MODE_DISABLED,
		AP_DFS_MODE_ENABLED,
	} ap_dfs_mode;
	int ap_ndpa_frame;

	const char *hostapd_debug_log;

#ifdef CONFIG_TRAFFIC_AGENT
	/* Traffic Agent */
	struct sigma_stream streams[MAX_SIGMA_STREAMS];
	int stream_id;
	int num_streams;
	pthread_t thr;
#endif /* CONFIG_TRAFFIC_AGENT */

	int throughput_pktsize; /* If non-zero, override pktsize for throughput
				 * tests */
	int no_timestamps;

	const char *sniffer_ifname;
	const char *set_macaddr;
	int tmp_mac_addr;
	int ap_is_dual;
	enum ap_mode ap_mode_1;
	enum ap_chwidth ap_chwidth_1;
	int ap_channel_1;
	char ap_countrycode[3];

	int ap_wpsnfc;

	enum ap_wme {
		AP_WME_OFF,
		AP_WME_ON,
	} ap_wme;

#ifdef CONFIG_SNIFFER
	pid_t sniffer_pid;
	char sniffer_filename[200];
#endif /* CONFIG_SNIFFER */

	int last_set_ip_config_ipv6;

	int tid_to_handle[8]; /* Mapping of TID to handle */
	int dialog_token; /* Used for generating unique handle for an addTs */

	enum sigma_program {
		PROGRAM_UNKNOWN = 0,
		PROGRAM_TDLS,
		PROGRAM_HS2,
		PROGRAM_HS2_R2,
		PROGRAM_WFD,
		PROGRAM_PMF,
		PROGRAM_WPS,
		PROGRAM_60GHZ,
		PROGRAM_HT,
		PROGRAM_VHT,
		PROGRAM_NAN,
	} program;

	enum device_type {
		device_type_unknown,
		AP_unknown,
		AP_testbed,
		AP_dut,
		STA_unknown,
		STA_testbed,
		STA_dut
	} device_type;

	enum {
		DEVROLE_UNKNOWN = 0,
		DEVROLE_STA,
		DEVROLE_PCP,
	} dev_role;

	const char *version;
	int no_ip_addr_set;
	int sta_channel;

	const char *summary_log;
	const char *hostapd_entropy_log;

	int iface_down_on_reset;
	int write_stats; /* traffic stream e2e*.txt files */
	int sim_no_username; /* do not set SIM username to use real SIM */
};


enum sigma_dut_print_level {
	DUT_MSG_DEBUG, DUT_MSG_INFO, DUT_MSG_ERROR
};

void sigma_dut_print(struct sigma_dut *dut, int level, const char *fmt, ...)
PRINTF_FORMAT(3, 4);

void sigma_dut_summary(struct sigma_dut *dut, const char *fmt, ...)
PRINTF_FORMAT(2, 3);


enum sigma_status {
	SIGMA_RUNNING, SIGMA_INVALID, SIGMA_ERROR, SIGMA_COMPLETE
};

void send_resp(struct sigma_dut *dut, struct sigma_conn *conn,
	       enum sigma_status status, char *buf);

const char * get_param(struct sigma_cmd *cmd, const char *name);

int sigma_dut_reg_cmd(const char *cmd,
		      int (*validate)(struct sigma_cmd *cmd),
		      int (*process)(struct sigma_dut *dut,
				     struct sigma_conn *conn,
				     struct sigma_cmd *cmd));

void sigma_dut_register_cmds(void);

int cmd_sta_send_frame(struct sigma_dut *dut, struct sigma_conn *conn,
		       struct sigma_cmd *cmd);
int cmd_sta_set_parameter(struct sigma_dut *dut, struct sigma_conn *conn,
			  struct sigma_cmd *cmd);
int cmd_ap_send_frame(struct sigma_dut *dut, struct sigma_conn *conn,
		      struct sigma_cmd *cmd);
int cmd_wlantest_send_frame(struct sigma_dut *dut, struct sigma_conn *conn,
			    struct sigma_cmd *cmd);

enum driver_type {
	DRIVER_NOT_SET,
	DRIVER_ATHEROS,
	DRIVER_WCN,
	DRIVER_MAC80211,
	DRIVER_AR6003,
	DRIVER_WIL6210,
	DRIVER_QNXNTO,
	DRIVER_OPENWRT
};

enum openwrt_driver_type {
	OPENWRT_DRIVER_NOT_SET,
	OPENWRT_DRIVER_ATHEROS
};

#define DRIVER_NAME_60G "wil6210"

int set_wifi_chip(const char *chip_type);
enum driver_type get_driver_type(void);
enum openwrt_driver_type get_openwrt_driver_type(void);
int file_exists(const char *fname);

struct wpa_ctrl;

int wps_connection_event(struct sigma_dut *dut, struct sigma_conn *conn,
			 struct wpa_ctrl *ctrl, const char *intf, int p2p_resp);
int ascii2hexstr(const char *str, char *hex);
void disconnect_station(struct sigma_dut *dut);
void nfc_status(struct sigma_dut *dut, const char *state, const char *oper);
int get_ip_config(struct sigma_dut *dut, const char *ifname, char *buf,
		  size_t buf_len);
int ath6kl_client_uapsd(struct sigma_dut *dut, const char *intf, int uapsd);
int is_ip_addr(const char *str);
int run_system(struct sigma_dut *dut, const char *cmd);
int cmd_wlantest_set_channel(struct sigma_dut *dut, struct sigma_conn *conn,
			     struct sigma_cmd *cmd);
void sniffer_close(struct sigma_dut *dut);

/* ap.c */
void ath_disable_txbf(struct sigma_dut *dut, const char *intf);
void ath_config_dyn_bw_sig(struct sigma_dut *dut, const char *ifname,
			   const char *val);
void novap_reset(struct sigma_dut *dut, const char *ifname);

/* sta.c */
int set_ps(const char *intf, struct sigma_dut *dut, int enabled);
void ath_set_zero_crc(struct sigma_dut *dut, const char *val);
void ath_set_cts_width(struct sigma_dut *dut, const char *ifname,
		       const char *val);
int ath_set_width(struct sigma_dut *dut, struct sigma_conn *conn,
		  const char *intf, const char *val);

/* p2p.c */
int p2p_cmd_sta_get_parameter(struct sigma_dut *dut, struct sigma_conn *conn,
			      struct sigma_cmd *cmd);

/* utils.h */
enum sigma_program sigma_program_to_enum(const char *prog);

/* uapsd_stream.c */
void receive_uapsd(struct sigma_stream *s);
void send_uapsd_console(struct sigma_stream *s);

/* nan.c */
int nan_preset_testparameters(struct sigma_dut *dut, struct sigma_conn *conn,
			      struct sigma_cmd *cmd);
int nan_cmd_sta_get_parameter(struct sigma_dut *dut, struct sigma_conn *conn,
			      struct sigma_cmd *cmd);
int nan_cmd_sta_exec_action(struct sigma_dut *dut, struct sigma_conn *conn,
			    struct sigma_cmd *cmd);
int nan_cmd_sta_get_events(struct sigma_dut *dut, struct sigma_conn *conn,
			   struct sigma_cmd *cmd);
int nan_cmd_sta_transmit_followup(struct sigma_dut *dut,
				  struct sigma_conn *conn,
				  struct sigma_cmd *cmd);
void nan_cmd_sta_reset_default(struct sigma_dut *dut, struct sigma_conn *conn,
			       struct sigma_cmd *cmd);
int nan_cmd_sta_preset_testparameters(struct sigma_dut *dut,
				      struct sigma_conn *conn,
				      struct sigma_cmd *cmd);

#endif /* SIGMA_DUT_H */