package com.xttechgroup.scaler.analyzerserv.models.Job;

import com.xttechgroup.scaler.analyzerserv.models.Job.nodes.ELFSymbolEntity;
import com.xttechgroup.scaler.analyzerserv.models.Job.relations.InvokeLib;
import com.xttechgroup.scaler.analyzerserv.models.Job.relations.InvokeSym;
import org.springframework.data.neo4j.core.schema.*;

import java.util.List;
import java.util.Set;

import static org.springframework.data.neo4j.core.schema.Relationship.Direction.OUTGOING;

@Node("CountingInfo")
public class TimingLibraryInfoEntity {
    @Id
    @GeneratedValue
    public Long id;

    @Property
    public Long fileID;

    //Symbol level counting and timing
    @Relationship(type = "INVOKED_SYM", direction = OUTGOING)
    List<InvokeSym> invokedSymbol;

    //Library level counting and timing
    @Relationship(type = "INVOKED_LIB", direction = OUTGOING)
    List<InvokeLib> invokedLibrary;

}
