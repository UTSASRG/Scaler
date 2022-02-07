package com.xttechgroup.scaler.analyzerserv.models.repository;

public interface ConfigRepo {
    String getConfigYaml(Long jobid);

    void setConfigYaml(Long jobid, String yamlConfig);
}
