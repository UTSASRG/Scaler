package com.xttechgroup.scaler.analyzerserv.models.nodes;

import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import lombok.Data;
import org.springframework.data.neo4j.core.schema.*;

import java.util.*;

// CREATE INDEX ON :ElfSym(scalerId)
// CREATE INDEX ON :ElfSym(libFileId)
// CREATE INDEX ON :ElfSym(hookedId)
// CREATE INDEX ON :ElfSym(hooked)
@Data
@Node("ElfSym")
public class ELFSymEntity {
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
    @Property
    Long hookedId;

    //Used for HAS_EXTSYM insertion internally
    //public long elfImgIdIndex;


    ELFSymEntity() {

    }

    /**
     * Convert protobuffer ELFSymbolInfoMsg to Map format for insertion to db.
     */
    public static Map<String, Object> protoToMap(ELFSymbolInfoMsg symbolInfoMsg, int elfImgIdIndex) {
        Map<String, Object> n1 = new HashMap<>();

        n1.put("scalerId", symbolInfoMsg.getScalerId());
        n1.put("symbolType", symbolInfoMsg.getSymbolType());
        n1.put("bindType", symbolInfoMsg.getBindType());
        n1.put("libFileId", symbolInfoMsg.getLibFileId());
        n1.put("gotAddr", symbolInfoMsg.getGotAddr());
        n1.put("hooked", symbolInfoMsg.getHooked());
        n1.put("symbolName", symbolInfoMsg.getSymbolName());
        n1.put("hookedId", symbolInfoMsg.getHookedId());

        //This is used to find the real elfimgid after insertion
        n1.put("elfImgId", elfImgIdIndex);
        return n1;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ELFSymEntity) {
            return id.equals(((ElfImgEntity) obj).id);
        } else {
            return false;
        }
    }


}
