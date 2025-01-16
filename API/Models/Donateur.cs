using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;

[Table("donateur")]
public class Donateur
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int ID { get; set; }
    public string volledige_naam { get; set; }
    public string email { get; set; }
    public string wachtwoord { get; set; }
}
