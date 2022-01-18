package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.ELFImgInfoMsg;
import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.annotation.Version;
import org.springframework.data.neo4j.core.schema.*;

import java.util.HashSet;
import java.util.Set;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("ElfImgInfo")
public class ElfImgInfoEntity {
    @Id
    String nodeId;

    @Property
    public long scalerId;
    @Property
    public String filePath;
    @Property
    public String symbolType;
    @Property
    public long addrStart;
    @Property
    public long addrEnd;
    @Property
    public long pltStartAddr;
    @Property
    public long pltSecStartAddr;
    @Relationship(type = "HAS", direction = OUTGOING)
    private Set<ELFSymbolEntity> symbolsInThisFile = new HashSet<>();

    public ElfImgInfoEntity(){

    }
    public ElfImgInfoEntity(Long jobid, ELFImgInfoMsg value) {
        this.scalerId = value.getScalerId();
        this.filePath = value.getFilePath();
        this.addrStart = value.getAddrStart();
        this.addrEnd = value.getAddrEnd();
        this.pltStartAddr = value.getPltStartAddr();
        this.pltSecStartAddr = value.getPltSecStartAddr();
        this.nodeId = jobid + "-" + this.scalerId;
        for (ELFSymbolInfoMsg elfSymbolInfo : value.getSymbolInfoInThisFileList()) {
            this.symbolsInThisFile.add(new ELFSymbolEntity(this.nodeId, elfSymbolInfo));
        }

    }

    public void setSymbolsInThisFile(Set<ELFSymbolEntity> symbolsInThisFile) {
        this.symbolsInThisFile = symbolsInThisFile;
    }

    public Set<ELFSymbolEntity> getSymbolsInThisFile() {
        return symbolsInThisFile;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ElfImgInfoEntity) {
            return nodeId.equals(((ElfImgInfoEntity) obj).nodeId);
        } else {
            return false;
        }
    }
}
