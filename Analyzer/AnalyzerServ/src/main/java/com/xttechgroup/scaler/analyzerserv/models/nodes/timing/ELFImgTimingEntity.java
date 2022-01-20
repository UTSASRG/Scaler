package com.xttechgroup.scaler.analyzerserv.models.nodes.timing;

import com.xttechgroup.scaler.analyzerserv.models.nodes.info.ElfImgInfoEntity;
import com.xttechgroup.scaler.analyzerserv.models.relations.InvokeImg;
import com.xttechgroup.scaler.analyzerserv.models.relations.InvokeSym;
import org.springframework.data.neo4j.core.schema.Node;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.Relationship;
import org.springframework.data.neo4j.core.schema.TargetNode;

import java.util.List;

@Node("ElfImgTiming")
public class ELFImgTimingEntity {

    @Relationship(type = "INVOKED_LIB", direction = Relationship.Direction.OUTGOING)
    List<InvokeImg> invokeImg;

    @Relationship(type = "INVOKED_SYM", direction = Relationship.Direction.OUTGOING)
    List<InvokeSym> invokeSyms;

    @Property
    ElfImgInfoEntity elfImgInfo;
}
