package panels;

import data.Task;

public class ParentTaskTreeTableNode extends TaskTreeTableNode {
    public ParentTaskTreeTableNode() {
        super(null, null);
    }

    public ParentTaskTreeTableNode(Task task) {
        super(null, task);
    }
}
