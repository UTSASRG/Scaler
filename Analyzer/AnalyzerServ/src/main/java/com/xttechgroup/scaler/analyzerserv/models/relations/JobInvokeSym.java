package com.xttechgroup.scaler.analyzerserv.models.relations;

import com.xttechgroup.scaler.analyzerserv.models.nodes.ELFSymEntity;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.RelationshipId;
import org.springframework.data.neo4j.core.schema.RelationshipProperties;
import org.springframework.data.neo4j.core.schema.TargetNode;

// CREATE INDEX ON :JobInvokeSym(threadID)
// CREATE INDEX ON :JobInvokeSym(processId)

@RelationshipProperties
public class JobInvokeSym {

    @RelationshipId
    private Long id;

    @Property
    Long counts;


    @Property
    Long threadId;

    @Property
    Long processId;

    @TargetNode
    ELFSymEntity symInfo;
}
