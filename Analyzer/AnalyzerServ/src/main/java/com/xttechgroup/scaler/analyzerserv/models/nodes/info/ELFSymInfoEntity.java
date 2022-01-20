package com.xttechgroup.scaler.analyzerserv.models.nodes.info;

import com.xttechgroup.scaler.analyzerserv.ELFSymbolInfoMsg;
import org.springframework.data.neo4j.core.schema.*;

import java.util.*;

@Node("ElfSymInfo")
public class ELFSymInfoEntity {
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

//    @Relationship(type = "HAS_SYM", direction = INCOMING)
//    public ElfImgInfoEntity elfImgInfoEntity;

    //Used for HAS_SYM insertion internally
    //public long elfImgIdIndex;


    ELFSymInfoEntity() {

    }

    /**
     * Convert protobuffer ELFSymbolInfoMsg to Map format for insertion to db.
     */
    public static Map<String, Object> protoToMap(ELFSymbolInfoMsg symbolInfoMsg, int elfImgIdIndex) {
        Map<String, Object> n1 = new HashMap<>();

        n1.put("scalerId", symbolInfoMsg.getScalerId());
        n1.put("symbolType", symbolInfoMsg.getSymbolType().name());
        n1.put("bindType", symbolInfoMsg.getBindType().name());
        n1.put("libFileId", symbolInfoMsg.getLibFileId());
        n1.put("gotAddr", symbolInfoMsg.getGotAddr());
        n1.put("hooked", symbolInfoMsg.getHooked());
        n1.put("symbolName", symbolInfoMsg.getSymbolName());

        //This is used to find the real elfimgid after insertion
        n1.put("elfImgId", elfImgIdIndex);
        return n1;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ELFSymInfoEntity) {
            return id.equals(((ElfImgInfoEntity) obj).id);
        } else {
            return false;
        }
    }


}
