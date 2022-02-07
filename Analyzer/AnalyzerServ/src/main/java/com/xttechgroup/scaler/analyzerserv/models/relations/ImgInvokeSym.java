package com.xttechgroup.scaler.analyzerserv.models.relations;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import com.xttechgroup.scaler.analyzerserv.models.nodes.ElfImgEntity;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.RelationshipId;
import org.springframework.data.neo4j.core.schema.RelationshipProperties;
import org.springframework.data.neo4j.core.schema.TargetNode;

// CREATE INDEX ON :ExtSymInvokeImg(threadID)
// CREATE INDEX ON :ExtSymInvokeImg(processId)

@RelationshipProperties
public class ImgInvokeSym {

    @RelationshipId
    private Long id;

    @Property
    Long duration;

    @Property
    Long threadId;

    @Property
    Long processId;


    @TargetNode
    ELFSymEntity symInfo;

}
