package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

@Data
public class UpdateConfigReqBody {

    public String yamlConfig = "";

    public String getYamlConfig() {
        return yamlConfig;
    }
}
