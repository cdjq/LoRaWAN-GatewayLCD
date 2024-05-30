CSRCS +=  
CPPSRCS += userUI.cpp GatewayUI_DS.cpp GatewayUI_https.cpp GatewayUI_wss.cpp log.cpp stacktrace.cpp

DEPPATH += --dep-path $(LVGL_DIR)/$(LVGL_DIR_NAME)/userUI
VPATH += :$(LVGL_DIR)/$(LVGL_DIR_NAME)/userUI 

CFLAGS += "-I/usr/include/curl" "-I/usr/include/libwebsockets" "-I$(LVGL_DIR)/$(LVGL_DIR_NAME)/userUI" 

LDFLAGS = -lcurl -lcJSON -pthread -lwebsockets -lmosquitto



