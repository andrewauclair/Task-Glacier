package tree;

import net.byteseek.swing.treetable.TableUtils;
import net.byteseek.swing.treetable.TreeTableModel;

import javax.swing.table.DefaultTableColumnModel;
import javax.swing.table.TableColumnModel;
import javax.swing.tree.TreeNode;

public class WeeklyReportTreeTableModel extends TreeTableModel {
    String[] dates = new String[7];

    public WeeklyReportTreeTableModel(TreeNode rootNode, String[] dates) {
        super(rootNode, false);

    }

    @Override
    public Object getColumnValue(TreeNode node, int column) {
        return null;
    }

    @Override
    protected TableColumnModel createTableColumnModel() {
        TableColumnModel result = new DefaultTableColumnModel();
        result.addColumn(TableUtils.createColumn(0, "Description"));
        result.addColumn(TableUtils.createColumn(8, "Time"));


        return result;
    }
}
