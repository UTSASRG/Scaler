package com.xttechgroup.scaler.analyzerserv.models.Job.relations;

import com.xttechgroup.scaler.analyzerserv.models.Job.TimingLibraryInfoEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.nodes.ELFSymbolEntity;
import org.springframework.data.neo4j.core.schema.Property;
import org.springframework.data.neo4j.core.schema.RelationshipId;
import org.springframework.data.neo4j.core.schema.RelationshipProperties;

import java.util.List;

@RelationshipProperties
public class InvokeSym {

    @RelationshipId
    private Long id;

    @Property
    Long duration;

    @Property
    Long counts;

    @Property
    ELFSymbolEntity symInfo;
}
