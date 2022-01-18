package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.annotation.Version;
import org.springframework.data.neo4j.core.schema.GeneratedValue;
import org.springframework.data.neo4j.core.schema.Id;
import org.springframework.data.neo4j.core.schema.Node;
import org.springframework.data.neo4j.core.schema.Property;

@Node("ELFSymbol")
public class ELFSymbolEntity {
    @Id
    String nodeId;
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

    public ELFSymbolEntity() {

    }

    public ELFSymbolEntity(String elfInfoNodeId, ELFSymbolInfoMsg rho) {
        this.scalerId = rho.getScalerId();
        this.symbolType = rho.getSymbolType().name();
        this.bindType = rho.getBindType().name();
        this.libFileId = rho.getLibFileId();
        this.gotAddr = rho.getGotAddr();
        this.hooked = rho.getHooked();
        this.nodeId = elfInfoNodeId + "-" + this.scalerId;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ELFSymbolEntity) {
            return nodeId.equals(((ElfImgInfoEntity) obj).nodeId);
        } else {
            return false;
        }
    }
}
