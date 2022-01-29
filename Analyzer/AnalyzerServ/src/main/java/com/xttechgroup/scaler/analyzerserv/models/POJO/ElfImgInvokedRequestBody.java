package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;
import org.springframework.context.annotation.Bean;

@Data
public class ElfImgInvokedRequestBody {
    public Long jobid = null;
    public Long[] elfImgIds = null;
}
