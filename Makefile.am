if AGM_NO_IPC
SUBDIRS = snd_parser service plugins/tinyalsa plugins/tinyalsa/test plugins/alsalib
all-local:
	@echo "Building with AGM_NO_IPC"
endif

if AGM_IPC_SWBINDER
SUBDIRS = snd_parser service ipc/SwBinders/agm_server ipc/SwBinders/agm_client plugins/tinyalsa
all-local:
	@echo "Building for AGM_IPC_SWBINDER"
endif

if AGM_IPC_DBUS
SUBDIRS = snd_parser service ipc/DBus/agm_client ipc/DBus/agm_server plugins/tinyalsa plugins/tinyalsa/test plugins/alsalib
all-local:
	@echo "Building with AGM_IPC_DBUS"
endif
