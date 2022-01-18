package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.ELFImgInfoMsg;
import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.neo4j.core.schema.*;

import java.util.HashSet;
import java.util.Set;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("ElfImgInfo")
public class ElfImgInfoEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Property
    public Long scalerId;
    @Property
    public String filePath;
    @Property
    public String symbolType;
    @Property
    public Long addrStart;
    @Property
    public Long addrEnd;
    @Property
    public Long pltStartAddr;
    @Property
    public Long pltSecStartAddr;
    @Relationship(type = "HAS", direction = OUTGOING)
    private Set<ELFSymbolEntity> symbolsInThisFile = new HashSet<>();

    public ElfImgInfoEntity(ELFImgInfoMsg value) {
        this.scalerId = value.getScalerId();
        this.filePath = value.getFilePath();
        this.addrStart = value.getAddrStart();
        this.addrEnd = value.getAddrEnd();
        this.pltStartAddr = value.getPltStartAddr();
        this.pltSecStartAddr = value.getPltSecStartAddr();
        for (ELFSymbolInfoMsg elfSymbolInfo : value.getSymbolInfoInThisFileList()) {
            this.symbolsInThisFile.add(new ELFSymbolEntity(elfSymbolInfo));
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
            return scalerId.equals(((ElfImgInfoEntity) obj).scalerId);
        } else {
            return false;
        }
    }
}
