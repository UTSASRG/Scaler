package com.xttechgroup.scaler.analyzerserv.models.POJO;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import lombok.Data;

@Data
public class ElfImgInfoQueryResult {
    public ElfImgEntity curImg;
    public Long totalSymNumber;
    public Long hookedSymNumber;
    public Long unhookedSymNumber;

    public ElfImgInfoQueryResult(ElfImgEntity curImg, Long totalSymNumber, Long hookedSymNumber) {
        this.curImg = curImg;
        this.totalSymNumber = totalSymNumber;
        this.hookedSymNumber = hookedSymNumber;
        this.unhookedSymNumber = totalSymNumber - hookedSymNumber;
    }
}
