package com.xttechgroup.scaler.analyzerserv.models.POJO;

import lombok.Data;

@Data
public class LibraryViewQueryResult {
    String elfImgName;
    Long cycles;

    public LibraryViewQueryResult(String elfImgName, Long cycles) {
        this.elfImgName = elfImgName;
        this.cycles = cycles;
    }
}
