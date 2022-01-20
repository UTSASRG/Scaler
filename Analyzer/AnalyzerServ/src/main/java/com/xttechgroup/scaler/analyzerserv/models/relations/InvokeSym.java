package com.xttechgroup.scaler.analyzerserv.models.relations;

import com.xttechgroup.scaler.analyzerserv.models.nodes.info.ELFSymInfoEntity;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.RelationshipId;
import org.springframework.data.neo4j.core.schema.RelationshipProperties;
import org.springframework.data.neo4j.core.schema.TargetNode;

@RelationshipProperties
public class InvokeSym {

    @RelationshipId
    private Long id;

    @Property
    Long duration;

    @Property
    Long counts;

    @TargetNode
    ELFSymInfoEntity symInfo;
}
