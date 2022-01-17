package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.neo4j.core.schema.Id;
import org.springframework.data.neo4j.core.schema.Node;
import org.springframework.data.neo4j.core.schema.Property;

@Node("ELFSymbol")
public class ELFSymbolEntity {
    @Id
    Long scalerId;
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
    Boolean hooked;

    public ELFSymbolEntity(ELFSymbolInfoMsg rho) {
        this.scalerId = rho.getScalerId();
        this.symbolType = rho.getSymbolType();
        this.bindType = rho.getBindType();
        this.libFileId = rho.getLibFileId();
        this.gotAddr = rho.getGotAddr();
        this.hooked = rho.getHooked();
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ELFSymbolEntity) {
            return scalerId.equals(((ELFSymbolEntity) obj).scalerId);
        } else {
            return false;
        }
    }
}
