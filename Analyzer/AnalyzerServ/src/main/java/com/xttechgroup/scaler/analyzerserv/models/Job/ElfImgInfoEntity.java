package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.ELFImgInfoMsg;
import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.neo4j.core.schema.*;

import java.util.*;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;
import static org.springframework.data.neo4j.core.schema.Relationship.Direction.INCOMING;

@Node("ElfImgInfo")
public class ElfImgInfoEntity {
    @Id
    @GeneratedValue
    public Long id;

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

    //    @Relationship(type = "HAS_IMG", direction = INCOMING)
    //    public JobEntity jobEntity;

    @Relationship(type = "HAS_SYM", direction = OUTGOING)
    private List<ELFSymbolEntity> symbolsInThisFile = new ArrayList<>();

    public ElfImgInfoEntity() {

    }

    /**
     * Convert protobuffer ELFSymbolInfoMsg to Map format for insertion to db.
     */
    public static Map<String, Object> protoToMap(ELFImgInfoMsg value, Long jobId) {
        Map<String, Object> n1 = new HashMap<>();

        n1.put("scalerId", value.getScalerId());
        n1.put("filePath", value.getFilePath());
        n1.put("addrStart", value.getAddrStart());
        n1.put("addrEnd", value.getAddrEnd());
        n1.put("pltStartAddr", value.getPltStartAddr());
        n1.put("pltSecStartAddr", value.getPltSecStartAddr());

        //Used for HAS_IMG relation
        n1.put("jobId", jobId);
        return n1;
    }

    public void setSymbolsInThisFile(List<ELFSymbolEntity> symbolsInThisFile) {
        this.symbolsInThisFile = symbolsInThisFile;
    }

    public List<ELFSymbolEntity> getSymbolsInThisFile() {
        return symbolsInThisFile;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ElfImgInfoEntity) {
            return id.equals(((ElfImgInfoEntity) obj).id);
        } else {
            return false;
        }
    }
}
