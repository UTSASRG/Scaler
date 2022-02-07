package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

@Data
public class ExecutionCommand {
    public boolean execute = false;
    public String yamlConfig;
}
