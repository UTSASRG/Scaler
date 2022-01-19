package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.ELFImgInfoMsg;
import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.annotation.Version;
import org.springframework.data.neo4j.core.schema.*;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.INCOMING;

@Node("ELFSymbol")
public class ELFSymbolEntity {
    @Id
    @GeneratedValue
    Long id;
    @Property
    long scalerId;
    @Property
    String symbolName;
    @Property
    String symbolType;
    @Property
    String bindType;
    @Property
    Long libFileId;
    @Property
    Long gotAddr;
    @Property
    boolean hooked;

    @Relationship(type = "HAS_SYM", direction = INCOMING)
    public ElfImgInfoEntity elfImgInfoEntity;

    ELFSymbolEntity() {

    }

    public ELFSymbolEntity(ElfImgInfoEntity elfImgInfoEntity, ELFSymbolInfoMsg rho) {
        this.scalerId = rho.getScalerId();
        this.symbolType = rho.getSymbolType().name();
        this.bindType = rho.getBindType().name();
        this.libFileId = rho.getLibFileId();
        this.gotAddr = rho.getGotAddr();
        this.hooked = rho.getHooked();
        this.symbolName = rho.getSymbolName();
        this.elfImgInfoEntity = elfImgInfoEntity;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ELFSymbolEntity) {
            return id.equals(((ElfImgInfoEntity) obj).id);
        } else {
            return false;
        }
    }


}
