package com.xttechgroup.scaler.analyzerserv.models.queries;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import lombok.Data;

@Data
public class ElfImgInfoQueryResult {
    public ElfImgEntity curImg;
    public Long totalSymNumber;
}
