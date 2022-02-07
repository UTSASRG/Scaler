package com.xttechgroup.scaler.analyzerserv.bean;

public class HelloMsg {
    private final String api;
    private final String appName;
    private final String version;
    private final String slogan;

    public HelloMsg(String api, String appName, String version, String slogan) {
        this.api = api;
        this.appName = appName;
        this.version = version;
        this.slogan = slogan;
    }

    public String getApi() {
        return api;
    }

    public String getAppName() {
        return appName;
    }

    public String getVersion() {
        return version;
    }

    public String getSlogan() {
        return slogan;
    }
}
