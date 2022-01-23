package com.xttechgroup.scaler.analyzerserv.models;

import com.xttechgroup.scaler.analyzerserv.models.relations.ImgInvokeSym;
import com.xttechgroup.scaler.analyzerserv.models.relations.JobInvokeSym;
import org.springframework.data.neo4j.core.schema.*;

import java.util.List;

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
    List<JobInvokeSym> invokedSymbol;

    //Library level counting and timing
    @Relationship(type = "INVOKED_LIB", direction = OUTGOING)
    List<ImgInvokeSym> invokedLibrary;

}
