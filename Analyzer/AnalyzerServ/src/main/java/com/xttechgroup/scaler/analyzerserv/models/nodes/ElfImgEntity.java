package com.xttechgroup.scaler.analyzerserv.models.nodes;

import com.fasterxml.jackson.databind.annotation.JsonAppend;
import com.xttechgroup.scaler.analyzerserv.ELFImgInfoMsg;
import com.xttechgroup.scaler.analyzerserv.models.relations.ImgInvokeSym;
import lombok.Data;
import org.springframework.data.neo4j.core.schema.*;

import java.util.*;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

// CREATE INDEX ON :ElfImg(scalerId)
// CREATE INDEX ON :ElfImg(jobID)

@Data
@Node("ElfImgInfo")
public class ElfImgEntity {
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
    @Property
    public boolean elfImgValid;

    @Relationship(type = "HAS_EXTSYM", direction = OUTGOING)
    public List<ELFSymEntity> symbolsInThisFile = new ArrayList<>();

    //Only stores timing info
    @Relationship(type = "ImgInvokeSym", direction = OUTGOING)
    private List<ImgInvokeSym> invokeSyms = new ArrayList<>();

    public ElfImgEntity() {

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
        n1.put("elfImgValid", value.getValid());
        n1.put("pltSecStartAddr", value.getPltSecStartAddr());
        //Used for HAS_IMG relation
        n1.put("jobId", jobId);
        return n1;
    }

    public void setSymbolsInThisFile(List<ELFSymEntity> symbolsInThisFile) {
        this.symbolsInThisFile = symbolsInThisFile;
    }

    public List<ELFSymEntity> getSymbolsInThisFile() {
        return symbolsInThisFile;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ElfImgEntity) {
            return id.equals(((ElfImgEntity) obj).id);
        } else {
            return false;
        }
    }
}
